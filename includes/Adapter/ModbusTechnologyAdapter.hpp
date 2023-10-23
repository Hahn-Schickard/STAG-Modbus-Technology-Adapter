#ifndef _MODBUS_TECHNOLOGY_ADAPTER_HPP
#define _MODBUS_TECHNOLOGY_ADAPTER_HPP

#include "internal/ModbusTechnologyAdapterImplementation.hpp"

namespace Technology_Adapter {

class ModbusTechnologyAdapter
    : public Technology_Adapter::TechnologyAdapterInterface {
public:
  ModbusTechnologyAdapter(std::string const& config_path);

  void start() override;
  void stop() override;

private:
  void interfaceSet() final;

  Modbus::ModbusTechnologyAdapterImplementation implementation_;
};

} // namespace Technology_Adapter

#endif //_MODBUS_TECHNOLOGY_ADAPTER_HPP
