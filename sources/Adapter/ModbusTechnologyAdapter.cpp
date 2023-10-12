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

  // `Bus::stop()` will call `cancelBus()` which erases from `buses_`,
  // so we iterate over a copy to avoid erase/iterate conflicts
  auto buses_copy = std::move(buses_);
  buses_.clear();
  for (auto& port_and_bus : buses_copy) {
    port_and_bus.second->stop();
  }

  port_finder_.stop();

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
    auto bus = Modbus::Bus::NonemptyPtr::make(*this, *config, actual_port,
        Technology_Adapter::NonemptyDeviceRegistryPtr(getDeviceRegistry()));
    auto map_pos = buses_.insert_or_assign(actual_port, bus).first;
    try {
      {
        std::lock_guard builder_lock(device_builder_mutex_);
        bus->buildModel(Information_Model::NonemptyDeviceBuilderInterfacePtr(
            getDeviceBuilder()));
      }
      bus->start();
    } catch (...) {
      buses_.erase(map_pos);
      throw;
    }
  } catch (std::runtime_error const&) {
    // already fine
    throw;
  } catch (std::exception const& exception) {
    throw std::runtime_error(
        "Unable to add bus " + actual_port + ": " + exception.what());
  }
}

void ModbusTechnologyAdapter::cancelBus(Modbus::Config::Portname const& port) {
  buses_.erase(port);
  port_finder_.unassign(port);
}

} // namespace Technology_Adapter
