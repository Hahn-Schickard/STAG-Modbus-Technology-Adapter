#ifndef _MODBUS_TECHNOLOGY_ADAPTER_INTERFACE_HPP
#define _MODBUS_TECHNOLOGY_ADAPTER_INTERFACE_HPP

#include "Config.hpp"

namespace Technology_Adapter::Modbus {

/**
 * @brief An abstract API for `ModbusTechnologyAdapterImplementation`
 *
 * It is abstract so that we can inject additional callbacks for testing.
 */
class ModbusTechnologyAdapterInterface {
public:
  virtual void start() = 0;
  virtual void stop() = 0;

  /**
   * @brief This is how `PortFinder` informs us of its successes
   *
   * @throws `std::runtime_error`
   */
  virtual void addBus(Config::Bus::NonemptyPtr const&,
      Config::Portname const& actual_port) = 0;

  /**
   * @brief Called when `Bus` communication fails
   */
  virtual void cancelBus(Config::Portname const&) = 0;
};

} // namespace Technology_Adapter::Modbus

#endif //_MODBUS_TECHNOLOGY_ADAPTER_INTERFACE_HPP
