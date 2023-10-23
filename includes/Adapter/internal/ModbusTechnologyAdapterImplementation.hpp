#ifndef _MODBUS_TECHNOLOGY_ADAPTER_IMPLEMENTATION_HPP
#define _MODBUS_TECHNOLOGY_ADAPTER_IMPLEMENTATION_HPP

#include <nlohmann/json.hpp>

#include "Bus.hpp"
#include "ModbusTechnologyAdapterInterface.hpp"
#include "PortFinder.hpp"

namespace Technology_Adapter::Modbus {

class ModbusTechnologyAdapterImplementation
    : public ModbusTechnologyAdapterInterface {
public:
  ModbusTechnologyAdapterImplementation(Modbus::Config::Buses);
  ModbusTechnologyAdapterImplementation(nlohmann::json const& config);
  ModbusTechnologyAdapterImplementation(
      ConstString::ConstString const& config_path);

  /// Must be called before any of the following methods
  void setInterfaces(
      Information_Model::NonemptyDeviceBuilderInterfacePtr device_builder,
      NonemptyDeviceRegistryPtr registry);

  void start() final;
  void stop() final;
  void addBus(Modbus::Config::Bus::NonemptyPtr,
      Modbus::Config::Portname const& actual_port) final;
  void cancelBus(Modbus::Config::Portname const&) final;

private:
  HaSLI::LoggerPtr const logger_;
  Information_Model::DeviceBuilderInterfacePtr device_builder_;
  DeviceRegistryPtr registry_;
  std::vector<Modbus::Config::Bus::NonemptyPtr> bus_configs_;
  Modbus::PortFinder port_finder_;
  std::map<Modbus::Config::Portname, Modbus::Bus::NonemptyPtr> buses_;
  Threadsafe::Resource<bool> stopping_{false};
  std::mutex device_builder_mutex_;
};

} // namespace Technology_Adapter::Modbus

#endif //_MODBUS_TECHNOLOGY_ADAPTER_IMPLEMENTATION_HPP
