#ifndef _MODBUS_TECHNOLOGY_ADAPTER_HPP
#define _MODBUS_TECHNOLOGY_ADAPTER_HPP

#include "LibmodbusAbstraction.hpp"
#include "Technology_Adapter_Interface/TechnologyAdapter.hpp"

namespace Modbus_Technology_Adapter {

class ModbusTechnologyAdapter : public Technology_Adapter::TechnologyAdapter {
public:
  ModbusTechnologyAdapter();

  void start() override;
  void stop() override;

private:
  void interfaceSet() final;

  LibModbus::ContextRTU bus_;
};

} // namespace Modbus_Technology_Adapter

#endif //_MODBUS_TECHNOLOGY_ADAPTER_HPP
