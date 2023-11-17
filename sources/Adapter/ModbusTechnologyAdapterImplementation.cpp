#include "internal/ModbusTechnologyAdapterImplementation.hpp"

#include "internal/ConfigJson.hpp"

namespace Technology_Adapter::Modbus {

ModbusTechnologyAdapterImplementation::ModbusTechnologyAdapterImplementation(
    LibModbus::Context::Factory context_factory,
    Modbus::Config::Buses bus_configs)
    : ModbusTechnologyAdapterInterface(),
      logger_(HaSLI::LoggerManager::registerLogger(
          "Modbus Adapter implementation")),
      bus_configs_(std::move(bus_configs)),
      context_factory_(std::move(context_factory)),
      port_finder_(*this, context_factory_) {

  logger_->info("Initializing Modbus Technology Adapter implementation");
}

ModbusTechnologyAdapterImplementation::ModbusTechnologyAdapterImplementation(
    LibModbus::Context::Factory context_factory, nlohmann::json const& config)
    : ModbusTechnologyAdapterImplementation(
          std::move(context_factory), Modbus::Config::BusesOfJson(config)) {}

ModbusTechnologyAdapterImplementation::ModbusTechnologyAdapterImplementation(
    LibModbus::Context::Factory context_factory,
    ConstString::ConstString const& config_path)
    : ModbusTechnologyAdapterImplementation(std::move(context_factory),
          Modbus::Config::loadConfig(config_path)) {}

void ModbusTechnologyAdapterImplementation::setInterfaces(
    Information_Model::NonemptyDeviceBuilderInterfacePtr const& device_builder,
    NonemptyDeviceRegistryPtr const& registry) {

  *device_builder_.lock() = device_builder.base();
  registry_ = registry.base();
}

void ModbusTechnologyAdapterImplementation::start() {
  /*
    We just need to start `port_finder_`, which at times calls `addBus`, which
    then does the rest.
  */
  port_finder_.addBuses(bus_configs_);
}

void ModbusTechnologyAdapterImplementation::stop() {
  *stopping_.lock() = true;
  // From now on, no thread can be in the main part of `addBus`

  /*
    Stopping all buses.
    We iterate over a copy of `buses_` so that we only need to lock for a short
    time, and thus concurrent `cancelBus` are still possible
  */
  std::map<Modbus::Config::Portname, Modbus::Bus::NonemptyPtr> buses_copy;
  {
    auto buses_access = buses_.lock();
    buses_copy = std::move(*buses_access);
    buses_access->clear();
  }
  for (auto& port_and_bus : buses_copy) {
    port_and_bus.second->stop();
  }

  port_finder_.stop();

  *stopping_.lock() = false;
}

void ModbusTechnologyAdapterImplementation::addBus(
    Modbus::Config::Bus::NonemptyPtr const& config,
    Modbus::Config::Portname const& actual_port) {

  auto stopping_access = stopping_.lock();
  if (*stopping_access) {
    // Don't add anything if we are already in the process of stopping
    return;
  }
  /*
    We keep holding the lock on `stopping_`: If another thread wants to enter
    the "stopping" stage, it has to wait for us to finish doing our damage so
    that, when it cleans stuff, it does not miss anything we've done.
  */

  logger_->info("Adding bus {} on port {}", config->id, actual_port);
  try {
    auto bus = Modbus::Bus::NonemptyPtr::make(*this, config, context_factory_,
        actual_port, Technology_Adapter::NonemptyDeviceRegistryPtr(registry_));
    auto map_pos = buses_.lock()->insert_or_assign(actual_port, bus).first;
    try {
      bus->start(Information_Model::NonemptyDeviceBuilderInterfacePtr(
          *device_builder_.lock()));
    } catch (...) {
      buses_.lock()->erase(map_pos);
      throw;
    }
  } catch (std::runtime_error const&) {
    // exception is already fine
    throw;
  } catch (std::exception const& exception) {
    throw std::runtime_error(std::string((std::string_view)(
        "Unable to add bus " + actual_port + ": " + exception.what())));
  }
}

void ModbusTechnologyAdapterImplementation::cancelBus(
    Modbus::Config::Portname const& port) {

  logger_->trace("Cancelling bus {}", port);

  // We want to lock `buses_` only for `std::map` operations, not for a
  // potential call to `~Bus`.
  // Otherwise, the following would be just `buses_.lock()->erase(port)`.
  {
    Threadsafe::SharedPtr<Bus> bus;
    {
      auto accessor = buses_.lock();
      auto iterator = accessor->find(port);
      bus = iterator->second.base();
      accessor->erase(iterator);
    }
  }

  port_finder_.unassign(port);
}

} // namespace Technology_Adapter::Modbus
