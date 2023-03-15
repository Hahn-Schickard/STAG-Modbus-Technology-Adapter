#ifndef _MODBUS_TECHNOLOGY_ADAPTER_BUS_HPP
#define _MODBUS_TECHNOLOGY_ADAPTER_BUS_HPP

#include "Technology_Adapter_Interface/TechnologyAdapter.hpp"
#include "Threadsafe_Containers/SharedPtr.hpp"

#include "Config.hpp"

namespace Modbus_Technology_Adapter {

class Bus : public Threadsafe::EnableSharedFromThis<Bus> {
public:
  Bus(Config::Device const&);

  void buildModel(
      NonemptyPointer::NonemptyPtr<Technology_Adapter::DeviceBuilderPtr> const&,
      NonemptyPointer::NonemptyPtr<
          Technology_Adapter::ModelRegistryPtr> const&);
  void start();
  void stop();

private:
  // This recursive method is local to `buildModel`.
  void buildGroup(
      NonemptyPointer::NonemptyPtr<Technology_Adapter::DeviceBuilderPtr> const&,
      std::string const&, // group id for `DeviceBuilderInterface`, "" for root
      NonemptyPointer::NonemptyPtr<Threadsafe::SharedPtr<Bus>> const&, // this
      Config::Group const&);

  Threadsafe::Resource<LibModbus::ContextRTU> context_;
  Config::Device config_;
};

using BusPtr = Threadsafe::SharedPtr<Bus>;

} // namespace Modbus_Technology_Adapter

#endif // _MODBUS_TECHNOLOGY_ADAPTER_BUS_HPP
