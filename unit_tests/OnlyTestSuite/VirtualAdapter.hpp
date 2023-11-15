#ifndef _MODBUS_TECHNOLOGY_ADAPTER_UNIT_TESTS_VIRTUAL_ADAPTER_HPP
#define _MODBUS_TECHNOLOGY_ADAPTER_UNIT_TESTS_VIRTUAL_ADAPTER_HPP

#include "internal/ModbusTechnologyAdapterInterface.hpp"

namespace ModbusTechnologyAdapterTests::VirtualAdapter {

class VirtualAdapter
    : public Technology_Adapter::Modbus::ModbusTechnologyAdapterInterface {

public:
  size_t start_called = 0;
  size_t stop_called = 0;
  size_t add_bus_called = 0;
  size_t cancel_bus_called = 0;

  void start() final;
  void stop() final;

  void addBus(Technology_Adapter::Modbus::Config::Bus::NonemptyPtr const&,
      Technology_Adapter::Modbus::Config::Portname const& actual_port) final;
  void cancelBus(Technology_Adapter::Modbus::Config::Portname const&) final;
};

} // namespace ModbusTechnologyAdapterTests::VirtualAdapter

#endif // _MODBUS_TECHNOLOGY_ADAPTER_UNIT_TESTS_VIRTUAL_ADAPTER_HPP
