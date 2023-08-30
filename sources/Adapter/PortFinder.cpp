#include "internal/PortFinder.hpp"

#include "ModbusTechnologyAdapter.hpp"

namespace Technology_Adapter::Modbus {

PortFinder::PortFinder(ModbusTechnologyAdapter& owner)
    : owner_(owner), plan_(PortFinderPlan::make()) {}

PortFinder::~PortFinder() {
  *destructing_.lock() = true;
  stop();
}

void PortFinder::addBuses(
    std::vector<Config::Bus::NonemptyPtr> const& new_buses) {

  addCandidates(plan_->addBuses(new_buses));
}

void PortFinder::stop() {
  auto ports_access = ports_.lock();
  for (auto& name_and_port : *ports_access) {
    name_and_port.second.stop();
  }
}

void PortFinder::addCandidates(PortFinderPlan::NewCandidates&& candidates) {
  if (*destructing_.lock()) {
    return;
  }
  for (auto const& candidate : candidates) {
    auto const& portname = candidate.getPort();
    auto ports_access = ports_.lock();
    auto port_emplace_result = ports_access->try_emplace(portname, portname,
        /*
          Here, we pass `this` to a lambda. Hence, a word about lifetimes.
          The lambda in question is only used in the search thread of some
          `Port` in `ports_`. In the destructor, we terminate all such threads
          while (using the `destructing_` flag) preventing the creation of
          further such threads.
        */
        [this](PortFinderPlan::Candidate const& candidate) {
          confirmCandidate(candidate);
        });
    auto& port = port_emplace_result.first->second;
    port.addCandidate(candidate);
  }
}

void PortFinder::confirmCandidate(PortFinderPlan::Candidate const& candidate) {
  addCandidates(candidate.confirm());
  owner_.addBus(candidate.getBus(), candidate.getPort());
}

} // namespace Technology_Adapter::Modbus
