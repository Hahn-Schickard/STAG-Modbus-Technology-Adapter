#ifndef _MODBUS_TECHNOLOGY_ADAPTER_IMPLEMENTATION_HPP
#define _MODBUS_TECHNOLOGY_ADAPTER_IMPLEMENTATION_HPP

#include <nlohmann/json.hpp>

#include "Technology_Adapter_Interface/TechnologyAdapterInterface.hpp"

#include "Bus.hpp"
#include "ModbusTechnologyAdapterInterface.hpp"
#include "PortFinder.hpp"

namespace Technology_Adapter::Modbus {

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
  Information_Model::DeviceBuilderInterfacePtr device_builder_;
  DeviceRegistryPtr registry_;
  LibModbus::Context::Factory context_factory_;
  Modbus::Config::Buses bus_configs_;
  Modbus::PortFinder port_finder_;
  Threadsafe::Resource<
      std::map<Modbus::Config::Portname, Modbus::Bus::NonemptyPtr>> buses_;
  Threadsafe::Resource<bool> stopping_{false};
  std::mutex device_builder_mutex_;
};

} // namespace Technology_Adapter::Modbus

#endif //_MODBUS_TECHNOLOGY_ADAPTER_IMPLEMENTATION_HPP
