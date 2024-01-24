#ifndef _MODBUS_TECHNOLOGY_ADAPTER_PORT_FINDER_HPP
#define _MODBUS_TECHNOLOGY_ADAPTER_PORT_FINDER_HPP

#include <HaSLL/Logger.hpp>
#include <Nonempty_Pointer/NonemptyPtr.hpp>
#include <Threadsafe_Containers/Resource.hpp>

#include "ModbusTechnologyAdapterInterface.hpp"
#include "Port.hpp"
#include "PortFinderPlan.hpp"

namespace Technology_Adapter {

class ModbusTechnologyAdapter;

namespace Modbus {

/**
 * @brief Manages port auto detection
 *
 * The actual searching is done by class `Port` per port.
 * The overall detection is coordinated by class `PortFinderPlan`.
 */
class PortFinder {
public:
  PortFinder() = delete;

  /// @pre The lifetime of `*this` is included in the lifetime of `owner`
  PortFinder(
      ModbusTechnologyAdapterInterface& owner, LibModbus::Context::Factory);

  /**
   * @brief Adds new buses to the search
   *
   * @pre All entries of `new_buses` are in fact new to the search
   */
  void addBuses(Config::Buses const& new_buses);

  /**
   * @brief Removes a bus-to-port assignment
   *
   * The consequence is that a new search thread starts for the port and that
   * the bus is (again) searched for on all feasible ports.
   *
   * @pre `port.assigned` for the respective `port`
   */
  void unassign(Modbus::Config::Portname const&);

  /**
   * @brief Stops all search threads and forgets all buses
   *
   * To recommence searching, one has to call `addBuses` again.
   */
  void reset();

private:
  // Organizes searches for the given candidates
  void addCandidates(PortFinderPlan::NewCandidates&&);

  // @pre `candidate.getPort().assigned`
  void confirmCandidate(PortFinderPlan::Candidate const& candidate);

  ModbusTechnologyAdapterInterface& owner_;
  LibModbus::Context::Factory context_factory_;
  NonemptyPointer::NonemptyPtr<Threadsafe::MutexSharedPtr<PortFinderPlan>>
      plan_;
  NonemptyPointer::NonemptyPtr<HaSLI::LoggerPtr> logger_;

  // The mutex ensures that `Port::addCandidate` and `Port::reset` are not
  // called concurrently
  Threadsafe::Resource<std::map<Config::Portname, Port>> ports_;
};

} // namespace Modbus
} // namespace Technology_Adapter

#endif // _MODBUS_TECHNOLOGY_ADAPTER_PORT_FINDER_HPP
