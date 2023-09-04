#ifndef _MODBUS_TECHNOLOGY_ADAPTER_BUS_HPP
#define _MODBUS_TECHNOLOGY_ADAPTER_BUS_HPP

#include "Nonempty_Pointer/NonemptyPtr.hpp"
#include "Technology_Adapter_Interface/TechnologyAdapterInterface.hpp"
#include "Threadsafe_Containers/QueuedMutex.hpp"
#include "Threadsafe_Containers/SharedPtr.hpp"

#include "Config.hpp"

namespace Technology_Adapter::Modbus {

class Bus : public Threadsafe::EnableSharedFromThis<Bus> {
public:
  using NonemptyPtr = NonemptyPointer::NonemptyPtr<Threadsafe::SharedPtr<Bus>>;

  /// May throw `std::bad_alloc`.
  /// May throw `LibModbus` with `errno == EINVAL` or `errno == ENOMEM`.
  Bus(Config::Bus const&, Config::Portname const&);

  void buildModel( //
      Information_Model::NonemptyDeviceBuilderInterfacePtr const&,
      Technology_Adapter::NonemptyDeviceRegistryPtr const&);
  void start();
  void stop();

private:
  // This recursive method is local to `buildModel`.
  // May throw `std::bad_alloc`.
  // @pre lifetime of `group` is contained in lifetime of `this`
  void buildGroup( //
      Information_Model::NonemptyDeviceBuilderInterfacePtr const&,
      std::string const&, // group id for `DeviceBuilderInterface`, "" for root
      NonemptyPtr const&, // `this`
      Config::Device const&, //
      RegisterSet const& holding_registers, //
      RegisterSet const& input_registers, //
      Config::Group const&);

  Config::Bus config_;
  NonemptyPointer::NonemptyPtr<HaSLI::LoggerPtr> logger_;
  Threadsafe::Resource<LibModbus::ContextRTU, Threadsafe::QueuedMutex> context_;
};

} // namespace Technology_Adapter::Modbus

#endif // _MODBUS_TECHNOLOGY_ADAPTER_BUS_HPP
