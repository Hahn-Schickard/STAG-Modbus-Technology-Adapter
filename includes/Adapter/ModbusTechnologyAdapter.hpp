#ifndef _MODBUS_TECHNOLOGY_ADAPTER_HPP
#define _MODBUS_TECHNOLOGY_ADAPTER_HPP

#include "Technology_Adapter_Interface/TechnologyAdapter.hpp"

#include "Bus.hpp"
#include "Config.hpp"
#include "LibmodbusAbstraction.hpp"

namespace Technology_Adapter {

class ModbusTechnologyAdapter : public Technology_Adapter::TechnologyAdapter {
public:
  ModbusTechnologyAdapter(Modbus::Config::Bus&&);

  void start() override;
  void stop() override;

private:
  void interfaceSet() final;

  NonemptyPointer::NonemptyPtr<Threadsafe::SharedPtr<Modbus::Bus>> bus_;
};

} // namespace Technology_Adapter

#endif //_MODBUS_TECHNOLOGY_ADAPTER_HPP
