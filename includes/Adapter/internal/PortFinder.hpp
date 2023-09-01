#ifndef _MODBUS_TECHNOLOGY_ADAPTER_PORT_FINDER_HPP
#define _MODBUS_TECHNOLOGY_ADAPTER_PORT_FINDER_HPP

#include <HaSLL/Logger.hpp>
#include <Nonempty_Pointer/NonemptyPtr.hpp>
#include <Threadsafe_Containers/Resource.hpp>

#include "internal/Port.hpp"
#include "internal/PortFinderPlan.hpp"

namespace Technology_Adapter {

class ModbusTechnologyAdapter;

namespace Modbus {

class PortFinder {
public:
  PortFinder() = delete;

  /// @pre The lifetime of `*this` is included in the lifetime of `owner`
  PortFinder(ModbusTechnologyAdapter& /*owner*/);

  ~PortFinder();

  /**
   * @brief Adds new buses to the search
   *
   * @pre All entries of `new_buses` are in fact new to the search
   */
  void addBuses(std::vector<Config::Bus::NonemptyPtr> const& /*new_buses*/);

  /// @brief Stops all search threads
  void stop();

private:
  void addCandidates(PortFinderPlan::NewCandidates&&);

  void confirmCandidate(PortFinderPlan::Candidate const&);

  ModbusTechnologyAdapter& owner_;
  PortFinderPlan::NonemptyPtr plan_;
  NonemptyPointer::NonemptyPtr<HaSLI::LoggerPtr> logger_;

  Threadsafe::Resource<std::map<Config::Portname, Port>> ports_;
  Threadsafe::Resource<bool> destructing_{false};
};

} // namespace Modbus
} // namespace Technology_Adapter

#endif // _MODBUS_TECHNOLOGY_ADAPTER_PORT_FINDER_HPP
