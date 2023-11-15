#ifndef _MODBUS_TECHNOLOGY_ADAPTER_INTERFACE_HPP
#define _MODBUS_TECHNOLOGY_ADAPTER_INTERFACE_HPP

#include "Config.hpp"

namespace Technology_Adapter::Modbus {

class ModbusTechnologyAdapterInterface {
public:
  virtual void start() = 0;
  virtual void stop() = 0;

  // @throws `std::runtime_error`
  virtual void addBus(Modbus::Config::Bus::NonemptyPtr const&,
      Modbus::Config::Portname const& actual_port) = 0;
  virtual void cancelBus(Modbus::Config::Portname const&) = 0;
};

} // namespace Technology_Adapter::Modbus

#endif //_MODBUS_TECHNOLOGY_ADAPTER_INTERFACE_HPP
