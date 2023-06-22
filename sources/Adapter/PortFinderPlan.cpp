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

bool operator<=(Config::Device const& smaller, Config::Device const& larger) {
  return (smaller.slave_id == larger.slave_id) &&
      (smaller.readable_registers <= larger.readable_registers);
}

bool operator<=(Config::Bus const& smaller, Config::Bus const& larger) {
  return std::all_of(smaller.devices.begin(), smaller.devices.end(),
      [&larger](Config::Device const& smaller_device) -> bool {
        return std::any_of(larger.devices.begin(), larger.devices.end(),
            [&smaller_device](Config::Device const& larger_device) -> bool {
              return smaller_device <= larger_device;
            });
      });
}

} // namespace Internal_

// `NonPortData`

PortFinderPlan::NonPortData::NonPortData()
    : bus_indexing(std::make_shared<Internal_::GlobalBusIndexing>()),
      ambiguates(bus_indexing, bus_indexing,
          [](Config::Bus::Ptr const& p1, Config::Bus::Ptr const& p2) -> bool {
            return (p1 != p2) && Internal_::operator<=(*p2, *p1);
          }) {}

// `Port`

PortFinderPlan::Port::Port(NonPortDataPtr const& non_port_data_)
    : non_port_data(non_port_data_) {}

PortFinderPlan::PortBusIndexing::Index PortFinderPlan::Port::addBus(
    Config::Bus::Ptr const& bus) {

  auto global_index = non_port_data->bus_indexing->index(bus);
  auto local_index = bus_indexing.add(bus);
  global_bus_index.set(local_index, global_index);

  auto& ambiguated_by_bus = ambiguated[local_index];
  for (auto other_local_index : bus_indexing) {
    if (other_local_index != local_index) {
      auto other_global_index = global_bus_index[other_local_index].value();
      if (non_port_data->ambiguates(global_index, other_global_index)) {
        ambiguated_by_bus.push_back(other_local_index);
      }
      if (non_port_data->ambiguates(other_global_index, global_index)) {
        ambiguated[other_local_index].push_back(local_index);
        if (available.contains(other_local_index)) {
          ++num_ambiguators[local_index];
        }
      }
    }
  }

  return local_index;
}

void PortFinderPlan::Port::makeBusAvailable(
    PortFinderPlan::PortBusIndexing::Index const& local_index) {

  auto global_index = global_bus_index[local_index].value();

  available.add(local_index);

  for (auto other_local_index : bus_indexing) {
    auto other_global_index = global_bus_index[other_local_index].value();
    if (non_port_data->ambiguates(global_index, other_global_index)) {
      ++num_ambiguators[other_local_index];
    }
  }
}

// `Candidate`:

Config::Bus::Ptr const& PortFinderPlan::Candidate::getBus() const {
  return plan_->non_port_data_->bus_indexing->get(
      plan_->ports_[port_].value().global_bus_index[bus_].value());
}

Config::Portname const& PortFinderPlan::Candidate::getPort() const {
  return plan_->non_port_data_->port_indexing.get(port_);
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

  std::vector<Internal_::GlobalBusIndexing::Index> new_global_indices;

  // append `buses` to `ports_by_name`
  for (auto const& bus : new_buses) {
    auto global_index = non_port_data_->bus_indexing->index(bus);
    new_global_indices.push_back(global_index);
    auto& possible_ports = non_port_data_->possible_ports[global_index];
    for (auto const& port_name : bus->possible_serial_ports) {
      auto port_index = non_port_data_->port_indexing.index(port_name);
      auto& maybe_port = ports_[port_index];
      if (!maybe_port.has_value()) {
        maybe_port.emplace(non_port_data_);
      }
      auto& port = maybe_port.value();
      auto local_index = port.addBus(bus);
      possible_ports.push_back(std::make_pair(port_index, local_index));
      port.makeBusAvailable(local_index);
    }
  }

  // detect candidates
  NewCandidates new_candidates;
  for (auto const& bus_global_index : new_global_indices) {
    auto bus = non_port_data_->bus_indexing->get(bus_global_index);
    for (auto& incidence : non_port_data_->possible_ports[bus_global_index]) {
      auto port_index = incidence.first;
      auto& port = ports_[port_index].value();
      auto local_index = incidence.second;
      if ((!port.assigned) && (port.num_ambiguators[local_index] == 0)) {
        Candidate new_candidate(
            PortFinderPlan::NonemptyPtr(shared_from_this()), port_index, local_index);
        new_candidates.push_back(std::move(new_candidate));
      }
    }
  }

  return new_candidates;
}

PortFinderPlan::NewCandidates PortFinderPlan::unassign(
    Config::Portname const& port_name) {

  auto port_index = non_port_data_->port_indexing.index(port_name);
  auto& port = ports_[port_index].value();
  if (!port.assigned) {
    return {};
  }

  Config::Bus::Ptr assigned_bus = port.assigned.value();
  auto assigned_bus_index = port.bus_indexing.lookup(assigned_bus);
  auto assigned_bus_global_index = port.global_bus_index[assigned_bus_index].value();

  port.assigned.reset();
  non_port_data_->assigned.remove(assigned_bus_global_index);
  NewCandidates new_candidates;

  // recall `assigned_bus` on other ports
  for (auto const& incidence : non_port_data_->possible_ports[assigned_bus_global_index]) {
    auto other_port_index = incidence.first;
    if (other_port_index != port_index) {
      auto& other_port = ports_[other_port_index].value();
      auto assigned_bus_other_index = incidence.second;
      other_port.available.add(assigned_bus_other_index);

      if ((!other_port.assigned) &&
          (other_port.num_ambiguators[assigned_bus_other_index] == 0)) {

        Candidate new_candidate(
            PortFinderPlan::NonemptyPtr(shared_from_this()),
            other_port_index, assigned_bus_other_index);
        new_candidates.push_back(std::move(new_candidate));
      }
    }
  }

  // detect candidates on `port`
  for (auto bus_index : port.bus_indexing) {
    if ((port.num_ambiguators[bus_index] == 0) &&
        !non_port_data_->assigned.contains(
            port.global_bus_index[bus_index].value())) {

      Candidate new_candidate(
          PortFinderPlan::NonemptyPtr(shared_from_this()), port_index, bus_index);
      new_candidates.push_back(std::move(new_candidate));
    }
  }

  // Retire existing candidates which are not unique any more
  for (auto const& incidence : non_port_data_->possible_ports[assigned_bus_global_index]) {
    auto other_port_index = incidence.first;
    auto& other_port = ports_[other_port_index].value();
    auto assigned_bus_other_index = incidence.second;
    for (auto ambiguated_index : other_port.ambiguated[assigned_bus_other_index]) {
      ++port.num_ambiguators[ambiguated_index];
    }
  }

  return new_candidates;
}

bool PortFinderPlan::feasible(
    PortBusIndexing::Index bus_index, PortIndexing::Index port_index) const {

  auto const& port = ports_[port_index].value();
  return (!port.assigned) && port.available.contains(bus_index) &&
      (port.num_ambiguators[bus_index] == 0);
}

PortFinderPlan::NewCandidates PortFinderPlan::assign(
    PortBusIndexing::Index bus_index, PortIndexing::Index actual_port_index) {

  auto bus_global_index =
      ports_[actual_port_index].value().global_bus_index[bus_index].value();

  non_port_data_->assigned.add(bus_global_index);
  NewCandidates new_candidates;

  // update ports
  for (auto const& incidence : non_port_data_->possible_ports[bus_global_index]) {
    auto some_port_index = incidence.first;
    auto& port = ports_[some_port_index].value();
    auto bus_local_index = incidence.second;
    if (some_port_index == actual_port_index) {
      port.assigned = non_port_data_->bus_indexing->get(bus_global_index);
    } else {
      port.available.remove(bus_local_index);

      // Check if anything became unambiguous
      for (auto ambiguated_index : port.ambiguated[bus_local_index]) {
        size_t& ambiguity = port.num_ambiguators[ambiguated_index];
        --ambiguity;
        if ((ambiguity == 0) && (!port.assigned) &&
            (!non_port_data_->assigned.contains(port.global_bus_index[ambiguated_index].value()))) {

          Candidate new_candidate(
              PortFinderPlan::NonemptyPtr(shared_from_this()),
              some_port_index, ambiguated_index);
          new_candidates.push_back(std::move(new_candidate));
        }
      }
    }
  }

  return new_candidates;
}

} // namespace Technology_Adapter::Modbus
