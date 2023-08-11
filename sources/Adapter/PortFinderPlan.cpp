#include "PortFinderPlan.hpp"

namespace Technology_Adapter::Modbus {

namespace Internal_ {

// Device comparison: registers are subsets
bool operator<=(Config::Device const& smaller, Config::Device const& larger) {
  return (smaller.slave_id == larger.slave_id) &&
      (smaller.holding_registers <= larger.holding_registers) &&
      (smaller.input_registers <= larger.input_registers);
}

// Bus comparison: all devices are subsets
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

// `GlobalData`

PortFinderPlan::GlobalData::GlobalData()
    : bus_indexing(std::make_shared<Internal_::GlobalBusIndexing>()),
      ambiguates(bus_indexing, bus_indexing,
          [](Config::Bus::NonemptyPtr const& p1,
              Config::Bus::NonemptyPtr const& p2) -> bool {
            return (p1 != p2) && Internal_::operator<=(*p2, *p1);
          }) {}

// `Port`

// NOLINTNEXTLINE(readability-identifier-naming)
PortFinderPlan::Port::Port(GlobalDataPtr global_data_)
    : global_data(std::move(global_data_)) {}

Internal_::GlobalBusIndexing::Index PortFinderPlan::Port::globalBusIndex(
    PortBusIndexing::Index index) const {

  // The invariant implies `has_value()`, obsoleting a check
  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
  return global_bus_index[index].value();
}

PortFinderPlan::PortBusIndexing::Index PortFinderPlan::Port::addBus(
    Internal_::GlobalBusIndexing::Index global_index) {

  auto const& bus = global_data->bus_indexing->get(global_index);
  auto local_index = bus_indexing.add(bus);
  global_bus_index.set(local_index, global_index);

  available.add(local_index);

  auto& ambiguated_by_bus = ambiguated[local_index];
  auto& num_bus_ambiguators = num_ambiguators[local_index];
  for (auto other_local_index : bus_indexing) {
    if (other_local_index != local_index) {
      auto other_global_index = globalBusIndex(other_local_index);
      if (global_data->ambiguates(global_index, other_global_index)) {
        ambiguated_by_bus.push_back(other_local_index);
        ++num_ambiguators[other_local_index];
      }
      if (global_data->ambiguates(other_global_index, global_index)) {
        ambiguated[other_local_index].push_back(local_index);
        if (available.contains(other_local_index)) {
          ++num_bus_ambiguators;
        }
      }
    }
  }

  return local_index;
}

// `Candidate`:

Config::Bus::NonemptyPtr const& PortFinderPlan::Candidate::getBus() const {
  std::lock_guard lock(plan_->mutex_);
  return plan_->global_data_->bus_indexing->get(
      plan_->getPort(port_).globalBusIndex(bus_));
}

Config::Portname const& PortFinderPlan::Candidate::getPort() const {
  std::lock_guard lock(plan_->mutex_);
  return plan_->global_data_->port_indexing.get(port_);
}

bool PortFinderPlan::Candidate::stillFeasible() const {
  std::lock_guard lock(plan_->mutex_);
  return plan_->feasible(bus_, port_);
}

PortFinderPlan::NewCandidates PortFinderPlan::Candidate::confirm() const {
  std::lock_guard lock(plan_->mutex_);
  return plan_->assign(bus_, port_);
}

// `PortFinderPlan`:

PortFinderPlan::PortFinderPlan(SecretConstructorArgument)
    : global_data_(std::make_shared<GlobalData>()) {}

PortFinderPlan::NewCandidates PortFinderPlan::addBuses(
    std::vector<Config::Bus::NonemptyPtr> const& new_buses) {

  std::lock_guard lock(mutex_);

  std::vector<Internal_::GlobalBusIndexing::Index> new_global_indices;

  // append `buses` to `ports_by_name`
  for (auto const& bus : new_buses) {
    auto global_index = global_data_->bus_indexing->add(bus);
    new_global_indices.push_back(global_index);
    auto& possible_ports = global_data_->possible_ports[global_index];
    for (auto const& port_name : bus->possible_serial_ports) {
      auto port_index = global_data_->port_indexing.index(port_name);

      // create/get the port
      auto& port_optional = ports_[port_index];
      if (!port_optional.has_value()) {
        port_optional.emplace(global_data_);
      }
      auto& port = port_optional.value();

      auto local_index = port.addBus(global_index);
      possible_ports.push_back(std::make_pair(port_index, local_index));
    }
  }

  // detect candidates
  NewCandidates new_candidates;
  for (auto const& bus_global_index : new_global_indices) {
    for (auto& incidence : global_data_->possible_ports[bus_global_index]) {
      considerCandidate(new_candidates, incidence.second, incidence.first);
    }
  }

  return new_candidates;
}

PortFinderPlan::NewCandidates PortFinderPlan::unassign(
    Config::Portname const& port_name) {

  std::lock_guard lock(mutex_);

  auto port_index = global_data_->port_indexing.lookup(port_name);
  auto& port = getPort(port_index);
  if (!port.assigned) {
    return {};
  }

  auto assigned_bus_index = port.assigned.value();
  auto assigned_bus_global_index = port.globalBusIndex(assigned_bus_index);

  port.assigned.reset();
  NewCandidates new_candidates;

  // recall `assigned_bus` on other ports
  for (auto const& incidence :
      global_data_->possible_ports[assigned_bus_global_index]) {

    auto other_port_index = incidence.first;
    if (other_port_index != port_index) {
      auto assigned_bus_other_index = incidence.second;
      auto& other_port = getPort(other_port_index);
      other_port.available.add(assigned_bus_other_index);
      considerCandidate(
          new_candidates, assigned_bus_other_index, other_port_index);
    }
  }

  // detect candidates on `port`
  for (auto bus_index : port.bus_indexing) {
    considerCandidate(new_candidates, bus_index, port_index);
  }

  // Retire existing candidates which are not unique any more
  for (auto const& incidence :
      global_data_->possible_ports[assigned_bus_global_index]) {

    auto other_port_index = incidence.first;
    if (other_port_index != port_index) {
      auto& other_port = getPort(other_port_index);
      auto assigned_bus_other_index = incidence.second;
      for (auto ambiguated_index :
          other_port.ambiguated[assigned_bus_other_index]) {

        ++other_port.num_ambiguators[ambiguated_index];
      }
    }
  }

  return new_candidates;
}

PortFinderPlan::NonemptyPtr PortFinderPlan::make() {
  return NonemptyPtr::make(SecretConstructorArgument());
}

bool PortFinderPlan::feasible(
    PortBusIndexing::Index bus_index, PortIndexing::Index port_index) const {

  auto const& port = getPort(port_index);
  return (!port.assigned) && port.available.contains(bus_index) &&
      (port.num_ambiguators[bus_index] == 0);
}

PortFinderPlan::Port const& PortFinderPlan::getPort(
    PortIndexing::Index index) const {

  // The invariant implies `has_value()`, obsoleting a check
  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
  return ports_[index].value();
}

PortFinderPlan::Port& PortFinderPlan::getPort(PortIndexing::Index index) {
  // The invariant implies `has_value()`, obsoleting a check
  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
  return ports_[index].value();
}

void PortFinderPlan::considerCandidate(NewCandidates& new_candidates,
    PortBusIndexing::Index bus_index, PortIndexing::Index port_index) {

  if (feasible(bus_index, port_index)) {
    new_candidates.emplace_back(SecretConstructorArgument(),
        PortFinderPlan::NonemptyPtr(shared_from_this()), port_index, bus_index);
  }
}

PortFinderPlan::NewCandidates PortFinderPlan::assign(
    PortBusIndexing::Index bus_index, PortIndexing::Index actual_port_index) {

  auto bus_global_index = getPort(actual_port_index).globalBusIndex(bus_index);

  NewCandidates new_candidates;

  // update ports
  for (auto const& incidence : global_data_->possible_ports[bus_global_index]) {
    auto other_port_index = incidence.first;
    auto& port = getPort(other_port_index);
    if (other_port_index == actual_port_index) {
      port.assigned = bus_index;
    } else {
      auto bus_other_index = incidence.second;
      port.available.remove(bus_other_index);

      // Check if anything became unambiguous
      for (auto ambiguated_index : port.ambiguated[bus_other_index]) {
        --port.num_ambiguators[ambiguated_index];
        considerCandidate(new_candidates, ambiguated_index, other_port_index);
      }
    }
  }

  return new_candidates;
}

} // namespace Technology_Adapter::Modbus
