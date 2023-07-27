#include "ModbusTechnologyAdapter.hpp"

namespace Technology_Adapter {

ModbusTechnologyAdapter::ModbusTechnologyAdapter(Modbus::Config::Bus&& config)
    : Technology_Adapter::TechnologyAdapterInterface("Modbus Adapter"),
      bus_(Modbus::Bus::NonemptyPtr::make(config)) {}

void ModbusTechnologyAdapter::interfaceSet() {}

void ModbusTechnologyAdapter::start() {
  bus_->buildModel(
      Information_Model::NonemptyDeviceBuilderInterfacePtr(getDeviceBuilder()),
      Technology_Adapter::NonemptyDeviceRegistryPtr(getDeviceRegistry()));

  bus_->start();
  Technology_Adapter::TechnologyAdapterInterface::start();
}

void ModbusTechnologyAdapter::stop() {
  Technology_Adapter::TechnologyAdapterInterface::stop();
  bus_->stop();
}

} // namespace Technology_Adapter
