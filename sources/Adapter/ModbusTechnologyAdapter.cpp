#include "ModbusTechnologyAdapter.hpp"

#include "internal/ConfigJson.hpp"

namespace Technology_Adapter {

ModbusTechnologyAdapter::ModbusTechnologyAdapter(std::string const& config_path)
    : Technology_Adapter::TechnologyAdapterInterface("Modbus Adapter"),
      implementation_(ConstString::ConstString(config_path)) {

  logger->info("Initializing Modbus Technology Adapter");
}

void ModbusTechnologyAdapter::start() {
  Technology_Adapter::TechnologyAdapterInterface::start();

  implementation_.start();
}

void ModbusTechnologyAdapter::stop() {
  implementation_.stop();

  Technology_Adapter::TechnologyAdapterInterface::stop();
}

void ModbusTechnologyAdapter::interfaceSet() {
  implementation_.setInterfaces(getDeviceBuilder(), getDeviceRegistry());
}

} // namespace Technology_Adapter
