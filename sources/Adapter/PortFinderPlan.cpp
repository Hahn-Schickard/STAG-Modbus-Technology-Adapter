#include "PortFinderPlan.hpp"

namespace Technology_Adapter::Modbus {

namespace Internal_ {

bool isDistinguishableFrom(
    Config::Device const& distinguished, Config::Device const& other) {

  return (distinguished.slave_id != other.slave_id) ||
      !(distinguished.readable_registers <= other.readable_registers);
}

bool isDistinguishableFrom(
    Config::Bus const& distinguished, Config::Bus const& other) {

  return std::any_of(distinguished.devices.begin(), distinguished.devices.end(),
      [&other](Config::Device const& distinguished_device) -> bool {
        return std::all_of(other.devices.begin(), other.devices.end(),
            [&distinguished_device](
                Config::Device const& other_device) -> bool {
              return isDistinguishableFrom(distinguished_device, other_device);
            });
      });
}

} // namespace Internal_

// `NonPortData`

PortFinderPlan::NonPortData::NonPortData()
    : bus_indexing(std::make_shared<Internal_::GlobalBusIndexing>()),
      distinguishable_from(bus_indexing, bus_indexing,
          [](Config::Bus::Ptr const& p1, Config::Bus::Ptr const& p2) -> bool {
            return Internal_::isDistinguishableFrom(*p1, *p2);
          }) {}

// `Port`

PortFinderPlan::Port::Port(NonPortDataPtr const& non_port_data_)
    : non_port_data(non_port_data_) {}

bool PortFinderPlan::Port::isBusUnique(Config::Bus::Ptr const& bus) const {
  auto bus_index = non_port_data->bus_indexing->index(bus);
  return (std::all_of(possible_buses.begin(), possible_buses.end(),
      [this, &bus_index](Config::Bus::Ptr const& candidate) -> bool {
        auto candidate_index = non_port_data->bus_indexing->index(candidate);
        return (candidate_index == bus_index) ||
            non_port_data->distinguishable_from(bus_index, candidate_index);
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
  return plan_->feasible(bus_, port_);
}

PortFinderPlan::NewCandidates PortFinderPlan::Candidate::confirm() {
  return plan_->assign(bus_, port_);
}

// `PortFinderPlan`:

PortFinderPlan::PortFinderPlan()
    : non_port_data_(std::make_shared<NonPortData>()) {}

PortFinderPlan::NewCandidates PortFinderPlan::addBuses(
    std::vector<Config::Bus::Ptr> const& new_buses) {

  // append `buses` to `ports_by_name`
  for (auto const& bus : new_buses) {
    for (auto const& port_name : bus->possible_serial_ports) {
      Port& port =
          ports_by_name_.try_emplace(port_name, non_port_data_).first->second;
      port.all_buses.push_back(bus);
      if (!port.assigned) {
        port.possible_buses.push_back(bus);
      }
    }
  }

  // detect candidates and ambiguity
  NewCandidates new_candidates;
  for (auto const& bus : new_buses) {
    for (auto const& port_name : bus->possible_serial_ports) {
      auto& port = ports_by_name_.at(port_name);
      if (!port.assigned) {
        if (port.isBusUnique(bus)) {
          Candidate new_candidate(
              bus, port_name, PortFinderPlan::NonemptyPtr(shared_from_this()));
          new_candidates.push_back(std::move(new_candidate));
        } else {
          port.ambiguous_buses.push_back(bus);
        }
      }
    }
  }

  // Retire existing candidates which are not unique any more
  for (auto& name_and_port : ports_by_name_) {
    auto& port = name_and_port.second;
    for (auto const& bus : port.possible_buses) {
      bool already_ambiguous =
          std::find(port.ambiguous_buses.begin(), port.ambiguous_buses.end(),
              bus) != port.ambiguous_buses.end();
      if ((!already_ambiguous) && (!port.isBusUnique(bus))) {
        port.ambiguous_buses.push_back(bus);
      }
    }
  }

  return new_candidates;
}

PortFinderPlan::NewCandidates PortFinderPlan::unassign(
    Config::Portname const& port_name) {

  auto& port = ports_by_name_.at(port_name);
  if (!port.assigned) {
    return {};
  }

  Config::Bus::Ptr assigned_bus = port.assigned.value();
  port.assigned.reset();
  NewCandidates new_candidates;

  // recall `assigned_bus` on other ports
  for (auto const& other_port_name : assigned_bus->possible_serial_ports) {
    if (other_port_name != port_name) {
      Port& other_port = ports_by_name_.at(other_port_name);
      if (!other_port.assigned) {
        other_port.possible_buses.push_back(assigned_bus);
        if (other_port.isBusUnique(assigned_bus)) {
          Candidate new_candidate(assigned_bus, other_port_name,
              PortFinderPlan::NonemptyPtr(shared_from_this()));
          new_candidates.push_back(std::move(new_candidate));
        } else {
          other_port.ambiguous_buses.push_back(assigned_bus);
        }
      }
    }
  }

  // recall buses on `port` (including `assigned_bus`)
  std::vector<Config::Bus::Ptr> recalled_buses;
  for (auto const& bus : port.all_buses) {
    bool assigned = std::any_of(ports_by_name_.cbegin(), ports_by_name_.cend(),
        [&bus](std::pair<Config::Portname, Port> const& entry) {
          return entry.second.assigned == bus;
        });
    if (!assigned) {
      recalled_buses.push_back(bus);
    }
  }
  port.possible_buses.insert(
      port.possible_buses.end(), recalled_buses.begin(), recalled_buses.end());

  // detect candidates and ambiguity on `port`
  for (auto const& bus : recalled_buses) {
    if (port.isBusUnique(bus)) {
      Candidate new_candidate(
          bus, port_name, PortFinderPlan::NonemptyPtr(shared_from_this()));
      new_candidates.push_back(std::move(new_candidate));
    } else {
      port.ambiguous_buses.push_back(bus);
    }
  }

  // Retire existing candidates which are not unique any more
  for (auto& name_and_port : ports_by_name_) {
    auto& other_port = name_and_port.second;
    for (auto const& bus : other_port.possible_buses) {
      bool already_ambiguous = //
          std::find(
              other_port.ambiguous_buses.begin(),
              other_port.ambiguous_buses.end(),
              bus) != other_port.ambiguous_buses.end();
      if ((!already_ambiguous) && (!other_port.isBusUnique(bus))) {
        other_port.ambiguous_buses.push_back(bus);
      }
    }
  }

  return new_candidates;
}

bool PortFinderPlan::feasible(
    Config::Bus::Ptr const& bus, Config::Portname const& port_name) const {

  auto const& port = ports_by_name_.at(port_name);
  bool possible =
      std::find(port.possible_buses.begin(), port.possible_buses.end(), bus) !=
      port.possible_buses.end();
  bool ambiguous =
      std::find(port.ambiguous_buses.begin(), port.ambiguous_buses.end(),
          bus) != port.ambiguous_buses.end();
  return possible && !ambiguous;
}

PortFinderPlan::NewCandidates PortFinderPlan::assign(
    Config::Bus::Ptr const& bus, Config::Portname const& actual_port_name) {

  NewCandidates new_candidates;

  // update ports_by_name
  for (auto const& some_port_name : bus->possible_serial_ports) {
    auto& port = ports_by_name_.at(some_port_name);
    if (some_port_name == actual_port_name) {
      port.assigned = bus;
      port.possible_buses.clear();
      port.ambiguous_buses.clear();
    } else {
      auto i = std::find(
          port.possible_buses.begin(), port.possible_buses.end(), bus);
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

  return new_candidates;
}

} // namespace Technology_Adapter::Modbus
