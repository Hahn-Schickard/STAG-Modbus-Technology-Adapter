#include "internal/PortFinder.hpp"

#include <HaSLL/LoggerManager.hpp>

#include "ModbusTechnologyAdapter.hpp"

namespace Technology_Adapter::Modbus {

PortFinder::PortFinder(ModbusTechnologyAdapterInterface& owner,
    LibModbus::Context::Factory context_factory)
    : owner_(owner), context_factory_(std::move(context_factory)),
      plan_(PortFinderPlan::make()),
      logger_(
          HaSLI::LoggerManager::registerLogger("Modbus Adapter port finder")) {}

PortFinder::~PortFinder() {
  *destructing_.lock() = true;
  stop();
}

void PortFinder::addBuses(Config::Buses const& new_buses) {
  logger_->info("Adding {} buses to the search", new_buses.size());
  addCandidates(plan_->addBuses(new_buses));
}

void PortFinder::unassign(Modbus::Config::Portname const& port) {
  {
    auto ports_access = ports_.lock();
    ports_access->find(port)->second.reset();
  }
  // Now we are already open for `addCandidates` from other threads.
  // Yet none will happen before the `unassign` in the next line.
  addCandidates(plan_->unassign(port));
}

void PortFinder::stop() {
  logger_->trace("Stopping");
  auto ports_access = ports_.lock();
  for (auto& name_and_port : *ports_access) {
    name_and_port.second.stop();
  }
  ports_access->clear();
  plan_ = PortFinderPlan::make();
}

void PortFinder::addCandidates(PortFinderPlan::NewCandidates&& candidates) {
  logger_->debug("Adding {} candidate(s)", candidates.size());
  if (*destructing_.lock()) {
    return;
  }
  for (auto const& candidate : candidates) {
    auto const& portname = candidate.getPort();
    auto ports_access = ports_.lock();
    auto port_emplace_result = ports_access->try_emplace( //
        portname, // key for the map
        context_factory_,
        portname, // first argument to `Port` constructor
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
  auto const& bus = candidate.getBus();
  auto const& port = candidate.getPort();
  logger_->info("Found bus {} on port {}", bus->id, port);
  addCandidates(candidate.confirm());
  try {
    owner_.addBus(bus, port);
  } catch (std::exception const& exception) {
    logger_->error(
        "While adding bus {} on port {}: {}", bus->id, port, exception.what());
    unassign(port);
  }
}

} // namespace Technology_Adapter::Modbus
