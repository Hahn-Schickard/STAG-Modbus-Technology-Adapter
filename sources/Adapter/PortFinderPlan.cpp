#include "PortFinderPlan.hpp"

namespace Technology_Adapter::Modbus {

namespace Internal_ {

bool isDistinguishableFrom(
    Config::Device const& distinguished, Config::Device const& other) {

  return (distinguished.slave_id != other.slave_id)
      || !(distinguished.readable_registers <= other.readable_registers);
}

bool isDistinguishableFrom(
    Config::Bus const& distinguished, Config::Bus const& other) {

  return std::any_of(distinguished.devices.begin(), distinguished.devices.end(),
      [&other](Config::Device const& distinguished_device) -> bool {
        return std::all_of(other.devices.begin(), other.devices.end(),
            [&distinguished_device](Config::Device const& other_device) -> bool
            {
              return isDistinguishableFrom(distinguished_device, other_device);
            });
      });
}

} // namespace Internal_

// `Port`

bool PortFinderPlan::Port::isBusUnique(Config::Bus::Ptr const& bus) const {
  return (std::all_of(
      possible_buses.begin(), possible_buses.end(),
      [&bus](Config::Bus::Ptr candidate) -> bool {
        return (candidate == bus)
            || Internal_::isDistinguishableFrom(*bus, *candidate);
      }));
}

// `Candidate`:

Config::Bus::Ptr const& PortFinderPlan::Candidate::getBus() const {
  return bus_;
}

Config::Portname const& PortFinderPlan::Candidate::getPort() const {
  return port_;
}

bool PortFinderPlan::Candidate::stillFeasible() const {
  return plan_->possible(bus_, port_);
}

PortFinderPlan::NewCandidates PortFinderPlan::Candidate::confirm() {
  return plan_->assign(bus_, port_);
}

// `PortFinderPlan`:

PortFinderPlan::NewCandidates PortFinderPlan::addBuses(
    std::vector<Config::Bus::Ptr> new_buses) {

  // append `buses` to `ports_by_name`
  for (auto const& bus : new_buses) {
    for (auto const& port_name : bus->possible_serial_ports) {
      Port& port = ports_by_name.try_emplace(port_name).first->second;
      port.possible_buses.push_back(bus);
    }
  }

  // append `buses` to `unassigned_buses`
  unassigned_buses.insert(
      unassigned_buses.cend(), new_buses.cbegin(), new_buses.cend());

  NewCandidates new_candidates;
  for (auto const& bus : new_buses) {
    for (auto const& port_name : bus->possible_serial_ports) {
      auto& port = ports_by_name.at(port_name);
      if (port.isBusUnique(bus)) {
        Candidate new_candidate(bus, port_name,
            PortFinderPlan::NonemptyPtr(shared_from_this()));
        new_candidates.push_back(std::move(new_candidate));
      } else {
        port.ambiguous_buses.push_back(bus);
      }
    }
  }
  return new_candidates;
}

bool PortFinderPlan::possible(
    Config::Bus::Ptr const& bus,
    Config::Portname const& port_name) const {

  auto& port = ports_by_name.at(port_name);
  
  return std::find(port.possible_buses.begin(), port.possible_buses.end(), bus)
      != port.possible_buses.end();
}

PortFinderPlan::NewCandidates PortFinderPlan::assign(
    Config::Bus::Ptr const& bus, Config::Portname const& actual_port_name) {

  NewCandidates new_candidates;

  // update ports_by_name
  for (auto const& some_port_name : bus->possible_serial_ports) {
    auto& port = ports_by_name.at(some_port_name);
    if (some_port_name == actual_port_name) {
      port.possible_buses.clear();
      port.ambiguous_buses.clear();
    } else {
      auto i = std::find(port.possible_buses.begin(), port.possible_buses.end(),
          bus);
      if (i != port.possible_buses.end()) { // possibly already removed
        port.possible_buses.erase(i);
      }

      // check if anything became unique
      auto current = port.ambiguous_buses.begin();
      while (current != port.ambiguous_buses.end()) {
        auto next = current;
        ++next;
        if (port.isBusUnique(*current)) {
          Candidate new_candidate(*current, some_port_name,
              PortFinderPlan::NonemptyPtr(shared_from_this()));
          new_candidates.push_back(std::move(new_candidate));
          port.ambiguous_buses.erase(current);
        }
        current = next;
      }
    }
  }

  // remove from `unassigned_buses`
  auto i = std::find(unassigned_buses.begin(), unassigned_buses.end(), bus);
  if (i == unassigned_buses.end()) {
    throw std::runtime_error("Invalid candidate");
  }
  unassigned_buses.erase(i);

  return new_candidates;
}

} // namespace Technology_Adapter::Modbus
