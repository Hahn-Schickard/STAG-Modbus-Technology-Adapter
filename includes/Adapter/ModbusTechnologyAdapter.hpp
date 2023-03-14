#ifndef _MODBUS_TECHNOLOGY_ADAPTER_HPP
#define _MODBUS_TECHNOLOGY_ADAPTER_HPP

#include "Technology_Adapter_Interface/TechnologyAdapter.hpp"

#include "Bus.hpp"
#include "Config.hpp"
#include "LibmodbusAbstraction.hpp"

namespace Modbus_Technology_Adapter {

class ModbusTechnologyAdapter : public Technology_Adapter::TechnologyAdapter {
public:
  ModbusTechnologyAdapter(Config::Device&&);

  void start() override;
  void stop() override;

private:
  void interfaceSet() final;

  Threadsafe::SharedPtr<Bus> bus_;
};

} // namespace Modbus_Technology_Adapter

#endif //_MODBUS_TECHNOLOGY_ADAPTER_HPP
