#ifndef _MODBUS_TECHNOLOGY_ADAPTER_HPP
#define _MODBUS_TECHNOLOGY_ADAPTER_HPP

#include "Technology_Adapter_Interface/TechnologyAdapterInterface.hpp"

#include "Config.hpp"
#include "internal/Bus.hpp"
#include "internal/PortFinder.hpp"

namespace Technology_Adapter {

class ModbusTechnologyAdapter
    : public Technology_Adapter::TechnologyAdapterInterface {
public:
  ModbusTechnologyAdapter(Modbus::Config::Buses const&);
  ModbusTechnologyAdapter(std::string const& config_path);

  void start() override;
  void stop() override;

private:
  void interfaceSet() final;

  // @throws `std::runtime_error`
  void addBus(Modbus::Config::Bus::NonemptyPtr,
      Modbus::Config::Portname const& actual_port);
  void cancelBus(Modbus::Config::Portname const&);

  std::vector<Modbus::Config::Bus::NonemptyPtr> bus_configs_;
  Modbus::PortFinder port_finder_;
  std::map<Modbus::Config::Portname, Modbus::Bus::NonemptyPtr> buses_;
  Threadsafe::Resource<bool> stopping_{false};
  std::mutex device_builder_mutex_;

  friend class Modbus::PortFinder;
  friend class Modbus::Bus;
};

} // namespace Technology_Adapter

#endif //_MODBUS_TECHNOLOGY_ADAPTER_HPP
