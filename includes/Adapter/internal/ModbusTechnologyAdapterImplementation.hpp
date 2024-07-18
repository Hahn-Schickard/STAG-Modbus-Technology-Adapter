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
  ModbusTechnologyAdapterImplementation(ModbusContext::Factory, Config::Buses);
  ModbusTechnologyAdapterImplementation(
      ModbusContext::Factory, nlohmann::json const& config);
  ModbusTechnologyAdapterImplementation(
      ModbusContext::Factory, ConstString::ConstString const& config_path);

  /// Must be called before any of the following methods
  void setInterfaces( //
      Information_Model::NonemptyDeviceBuilderInterfacePtr const&
          device_builder,
      NonemptyDeviceRegistryPtr const& registry);

  void start() override;
  void stop() override;
  void addBus(Config::Bus::NonemptyPtr const&,
      Config::Portname const& actual_port) override;
  void cancelBus(Config::Portname const&) override;

private:
  HaSLI::LoggerPtr const logger_;
  Config::Buses const bus_configs_; // used during `start`
  Threadsafe::PrivateResource<Information_Model::DeviceBuilderInterfacePtr>
      device_builder_;
  DeviceRegistryPtr registry_;
  ModbusContext::Factory context_factory_;
  PortFinder port_finder_;

  // Holds all running `Bus`es so we know what to stop when we stop.
  // `Bus` objects are deleted upon, both, `stop` and `cancelBus`. Re-starting a
  // bus through `addBus` will construct a new `Bus` object.
  Threadsafe::PrivateResource<std::map<Config::Portname, Bus::NonemptyPtr>>
      buses_;

  /*
    Used for synchronization of `stop` with `addBus`

    In fact, `addBus` is no-op while `stop` runs. Once `stop` is finished,
    `port_finder_` is stopped, so there is noone to call `addBus` any more
    before the next `start`.
  */
  Threadsafe::PrivateResource<bool> stopping_{false};
};

} // namespace Technology_Adapter::Modbus

#endif //_MODBUS_TECHNOLOGY_ADAPTER_IMPLEMENTATION_HPP
