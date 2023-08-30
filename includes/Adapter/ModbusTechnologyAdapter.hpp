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

  void start() override;
  void stop() override;

private:
  void interfaceSet() final;
  void addBus(Modbus::Config::Bus::NonemptyPtr,
      Modbus::Config::Portname const& /*actual_port*/);

  std::vector<Modbus::Config::Bus::NonemptyPtr> bus_configs_;
  Modbus::PortFinder port_finder_;
  std::vector<Modbus::Bus::NonemptyPtr> buses_;
  Threadsafe::Resource<bool> stopping_{false};

  friend class Modbus::PortFinder;
};

} // namespace Technology_Adapter

#endif //_MODBUS_TECHNOLOGY_ADAPTER_HPP
