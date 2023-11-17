#ifndef _MODBUS_TECHNOLOGY_ADAPTER_IMPLEMENTATION_HPP
#define _MODBUS_TECHNOLOGY_ADAPTER_IMPLEMENTATION_HPP

#include <nlohmann/json.hpp>

#include "Technology_Adapter_Interface/TechnologyAdapterInterface.hpp"

#include "Bus.hpp"
#include "ModbusTechnologyAdapterInterface.hpp"
#include "PortFinder.hpp"

namespace Technology_Adapter::Modbus {

/**
 * @brief The actual implementation for `ModbusTechnologyAdapter`
 *
 * `ModbusTechnologyAdapter` forwards all calls to this class.
 */
class ModbusTechnologyAdapterImplementation
    : public ModbusTechnologyAdapterInterface {
public:
  ModbusTechnologyAdapterImplementation(
      LibModbus::Context::Factory, Modbus::Config::Buses);
  ModbusTechnologyAdapterImplementation(
      LibModbus::Context::Factory, nlohmann::json const& config);
  ModbusTechnologyAdapterImplementation(
      LibModbus::Context::Factory, ConstString::ConstString const& config_path);

  /// Must be called before any of the following methods
  void setInterfaces(
      Information_Model::NonemptyDeviceBuilderInterfacePtr const&
          device_builder,
      NonemptyDeviceRegistryPtr const& registry);

  void start() override;
  void stop() override;
  void addBus(Modbus::Config::Bus::NonemptyPtr const&,
      Modbus::Config::Portname const& actual_port) override;
  void cancelBus(Modbus::Config::Portname const&) override;

private:
  HaSLI::LoggerPtr const logger_;
  Modbus::Config::Buses const bus_configs_; // used during `start`
  Threadsafe::Resource<Information_Model::DeviceBuilderInterfacePtr>
      device_builder_;
  DeviceRegistryPtr registry_;
  LibModbus::Context::Factory context_factory_;
  Modbus::PortFinder port_finder_;

  // Holds all running `Bus`es so we know what to stop when we stop
  Threadsafe::Resource<
      std::map<Modbus::Config::Portname, Modbus::Bus::NonemptyPtr>> buses_;

  /*
    Used for synchronization of `stop` with `addBus`

    In fact, `addBus` is no-op while `stop` runs. Once `stop` is finished,
    `port_finder_` is stopped, so there is noone to call `addBus` any more
    before the next `start`.
  */
  Threadsafe::Resource<bool> stopping_{false};
};

} // namespace Technology_Adapter::Modbus

#endif //_MODBUS_TECHNOLOGY_ADAPTER_IMPLEMENTATION_HPP
