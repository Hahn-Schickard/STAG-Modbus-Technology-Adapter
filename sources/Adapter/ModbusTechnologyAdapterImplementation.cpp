#include "internal/ModbusTechnologyAdapterImplementation.hpp"

#include "internal/ConfigJson.hpp"

namespace Technology_Adapter::Modbus {

ModbusTechnologyAdapterImplementation::ModbusTechnologyAdapterImplementation(
    Modbus::Config::Buses bus_configs)
    : ModbusTechnologyAdapterInterface(),
      logger_(HaSLI::LoggerManager::registerLogger(
          "Modbus Adapter implementation")),
      bus_configs_(std::move(bus_configs)), port_finder_(*this) {

  logger_->info("Initializing Modbus Technology Adapter");
}

ModbusTechnologyAdapterImplementation::ModbusTechnologyAdapterImplementation(
    nlohmann::json const& config)
    : ModbusTechnologyAdapterImplementation(
          Modbus::Config::BusesOfJson(config)) {}

ModbusTechnologyAdapterImplementation::ModbusTechnologyAdapterImplementation(
    ConstString::ConstString const& config_path)
    : ModbusTechnologyAdapterImplementation(
          Modbus::Config::loadConfig(config_path)) {}

void ModbusTechnologyAdapterImplementation::setInterfaces(
    Information_Model::NonemptyDeviceBuilderInterfacePtr const& device_builder,
    NonemptyDeviceRegistryPtr const& registry) {

  device_builder_ = device_builder.base();
  registry_ = registry.base();
}

void ModbusTechnologyAdapterImplementation::start() {
  port_finder_.addBuses(bus_configs_);
}

void ModbusTechnologyAdapterImplementation::stop() {
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
}

void ModbusTechnologyAdapterImplementation::addBus(
    Modbus::Config::Bus::NonemptyPtr config,
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

  logger_->info("Adding bus {} on port {}", config->id, actual_port);
  try {
    auto bus = Modbus::Bus::NonemptyPtr::make(*this, *config,
        LibModbus::ContextRTU::make, actual_port,
        Technology_Adapter::NonemptyDeviceRegistryPtr(registry_));
    auto map_pos = buses_.insert_or_assign(actual_port, bus).first;
    try {
      bus->start();
      {
        std::lock_guard builder_lock(device_builder_mutex_);
        bus->buildModel(Information_Model::NonemptyDeviceBuilderInterfacePtr(
            device_builder_));
      }
    } catch (...) {
      buses_.erase(map_pos);
      throw;
    }
  } catch (std::runtime_error const&) {
    // already fine
    throw;
  } catch (std::exception const& exception) {
    throw std::runtime_error(std::string((std::string_view)(
        "Unable to add bus " + actual_port + ": " + exception.what())));
  }
}

void ModbusTechnologyAdapterImplementation::cancelBus(
    Modbus::Config::Portname const& port) {

  logger_->trace("Cancelling bus {}", port);
  buses_.erase(port);
  port_finder_.unassign(port);
}

} // namespace Technology_Adapter::Modbus
