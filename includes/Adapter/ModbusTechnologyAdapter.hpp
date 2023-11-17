#ifndef _MODBUS_TECHNOLOGY_ADAPTER_HPP
#define _MODBUS_TECHNOLOGY_ADAPTER_HPP

#include "internal/ModbusTechnologyAdapterImplementation.hpp"

namespace Technology_Adapter {

/// @brief A Technology Adapter for the Modbus protocol
class ModbusTechnologyAdapter
    : public Technology_Adapter::TechnologyAdapterInterface {
public:
  /// @brief The config format is described in `internal/ConfigJson.hpp`
  ModbusTechnologyAdapter(std::string const& config_path);

  void start() final;
  void stop() final;

private:
  void interfaceSet() final;

  Modbus::ModbusTechnologyAdapterImplementation implementation_;
};

} // namespace Technology_Adapter

#endif //_MODBUS_TECHNOLOGY_ADAPTER_HPP
