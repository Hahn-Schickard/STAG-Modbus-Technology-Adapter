#include "ModbusTechnologyAdapter.hpp"

namespace Modbus_Technology_Adapter {

ModbusTechnologyAdapter::ModbusTechnologyAdapter(Config::Device&& config)
    : Technology_Adapter::TechnologyAdapter("Modbus Adapter"),
      bus_(BusPtr::make(config)) {}

void ModbusTechnologyAdapter::interfaceSet() {

  // model the device in `builder_interface_`

  bus_->buildModel(getDeviceBuilder(), getModelRegistry());
}

void ModbusTechnologyAdapter::start() {
  bus_->start();
  Technology_Adapter::TechnologyAdapter::start();
}

void ModbusTechnologyAdapter::stop() {
  Technology_Adapter::TechnologyAdapter::stop();
  bus_->stop();
}

} // namespace Modbus_Technology_Adapter
