#include "ModbusTechnologyAdapter.hpp"

#include "ConfigJson.hpp"

namespace Technology_Adapter {

ModbusTechnologyAdapter::ModbusTechnologyAdapter(
    Modbus::Config::Buses const& bus_configs)
    : Technology_Adapter::TechnologyAdapterInterface("Modbus Adapter"),
      port_finder_(*this) {

  logger->info("Initializing Modbus Technology Adapter");
  bus_configs_.reserve(bus_configs.size());
  for (auto const& bus_config : bus_configs) {
    bus_configs_.push_back(Modbus::Config::Bus::NonemptyPtr::make(bus_config));
  }
}

ModbusTechnologyAdapter::ModbusTechnologyAdapter(std::string const& config_path)
    : ModbusTechnologyAdapter(Modbus::Config::loadConfig(config_path)) {}

void ModbusTechnologyAdapter::interfaceSet() {}

void ModbusTechnologyAdapter::start() {
  Technology_Adapter::TechnologyAdapterInterface::start();

  port_finder_.addBuses(bus_configs_);
}

void ModbusTechnologyAdapter::stop() {
  *stopping_.lock() = true;

  port_finder_.stop();

  for (auto& bus : buses_) {
    bus->stop();
  }
  buses_.clear();

  *stopping_.lock() = false;

  Technology_Adapter::TechnologyAdapterInterface::stop();
}

void ModbusTechnologyAdapter::addBus(Modbus::Config::Bus::NonemptyPtr config,
    Modbus::Config::Portname const& actual_port) {

  auto stopping_access = stopping_.lock();
  if (*stopping_access) {
    // Don't add anything if we are already in the process of stopping
    return;
  }
  /*
    We keep holding the lock on `stopping_`: If another thread wants to enter
    the "stopping" stage, it has to wait for us to finish doing our damage so
    that, when it cleans stuff, it does not miss anything.
  */

  logger->info("Adding bus {} on port {}", config->id, actual_port);
  try {
    auto bus = Modbus::Bus::NonemptyPtr::make(*config, actual_port);
    buses_.push_back(bus);
    {
      std::lock_guard builder_lock(device_builder_mutex_);
      bus->buildModel( //
          Information_Model::NonemptyDeviceBuilderInterfacePtr(
              getDeviceBuilder()),
          Technology_Adapter::NonemptyDeviceRegistryPtr(getDeviceRegistry()));
    }
    bus->start();
  } catch (std::runtime_error const&) {
    // already fine
    throw;
  } catch (std::exception const& exception) {
    throw std::runtime_error(std::string((std::string_view)(
        "Unable to add bus " + actual_port + ": " + exception.what())));
  }
}

} // namespace Technology_Adapter
