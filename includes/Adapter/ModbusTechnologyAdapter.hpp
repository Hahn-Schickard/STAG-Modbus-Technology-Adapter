#ifndef _MODBUS_TECHNOLOGY_ADAPTER_HPP
#define _MODBUS_TECHNOLOGY_ADAPTER_HPP

#include "Config.hpp"
#include "LibmodbusAbstraction.hpp"
#include "Technology_Adapter_Interface/TechnologyAdapter.hpp"

namespace Modbus_Technology_Adapter {

class ModbusTechnologyAdapter : public Technology_Adapter::TechnologyAdapter {
public:
  ModbusTechnologyAdapter(Config::Device&&);

  void start() override;
  void stop() override;

private:
  void interfaceSet() final;

  // This recursive method is local to `interfaceSet`.
  void registerSubgroupContents(Technology_Adapter::DeviceBuilderPtr const&,
      std::string const&, // group ref id for `DeviceBuilderInterface`
      Config::Group const&);

  LibModbus::ContextRTU bus_;
  Config::Device config_;
};

} // namespace Modbus_Technology_Adapter

#endif //_MODBUS_TECHNOLOGY_ADAPTER_HPP
