#ifndef _MODBUS_TECHNOLOGY_ADAPTER_BUS_HPP
#define _MODBUS_TECHNOLOGY_ADAPTER_BUS_HPP

#include "Technology_Adapter_Interface/TechnologyAdapter.hpp"
#include "Threadsafe_Containers/QueuedMutex.hpp"
#include "Threadsafe_Containers/SharedPtr.hpp"

#include "Config.hpp"

namespace Technology_Adapter::Modbus {

class Bus : public Threadsafe::EnableSharedFromThis<Bus> {
public:
  Bus(Config::Bus const&);

  void buildModel(
      NonemptyPointer::NonemptyPtr<Technology_Adapter::DeviceBuilderPtr> const&,
      NonemptyPointer::NonemptyPtr<
          Technology_Adapter::ModelRegistryPtr> const&);
  void start();
  void stop();

private:
  // This recursive method is local to `buildModel`.
  // @pre lifetime of `group` is contained in lifetime of `this`
  void buildGroup(
      NonemptyPointer::NonemptyPtr<Technology_Adapter::DeviceBuilderPtr> const&,
      std::string const&, // group id for `DeviceBuilderInterface`, "" for root
      NonemptyPointer::NonemptyPtr<Threadsafe::SharedPtr<Bus>> const&,
      Config::Device const&, //
      RegisterSet const& /*readable_registers*/, //
      Config::Group const& /*group*/);

  Threadsafe::Resource<LibModbus::ContextRTU, Threadsafe::QueuedMutex> context_;
  Config::Bus config_;
};

using BusPtr = Threadsafe::SharedPtr<Bus>;

} // namespace Technology_Adapter::Modbus

#endif // _MODBUS_TECHNOLOGY_ADAPTER_BUS_HPP
