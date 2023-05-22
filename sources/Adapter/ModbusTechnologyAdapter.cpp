#include "ModbusTechnologyAdapter.hpp"

namespace Technology_Adapter {

ModbusTechnologyAdapter::ModbusTechnologyAdapter(Modbus::Config::Bus&& config)
    : Technology_Adapter::TechnologyAdapter("Modbus Adapter"),
      bus_(Modbus::BusPtr::make(config)) {}

void ModbusTechnologyAdapter::interfaceSet() {

  // model the device in `builder_interface_`

  bus_->buildModel(
      NonemptyPointer::NonemptyPtr<Technology_Adapter::DeviceBuilderPtr>(
          getDeviceBuilder()),
      NonemptyPointer::NonemptyPtr<Technology_Adapter::ModelRegistryPtr>(
          getModelRegistry()));
}

void ModbusTechnologyAdapter::start() {
  bus_->start();
  Technology_Adapter::TechnologyAdapter::start();
}

void ModbusTechnologyAdapter::stop() {
  Technology_Adapter::TechnologyAdapter::stop();
  bus_->stop();
}

} // namespace Technology_Adapter
