#include "VirtualAdapter.hpp"

namespace ModbusTechnologyAdapterTests::VirtualAdapter {

void VirtualAdapter::start() { ++start_called; }
void VirtualAdapter::stop() { ++stop_called; }

void VirtualAdapter::addBus(
    Technology_Adapter::Modbus::Config::Bus::NonemptyPtr,
    Technology_Adapter::Modbus::Config::Portname const&) {

  ++add_bus_called;
}

void VirtualAdapter::cancelBus(
    Technology_Adapter::Modbus::Config::Portname const&) {

  ++cancel_bus_called;
}

} // namespace ModbusTechnologyAdapterTests::VirtualAdapter
