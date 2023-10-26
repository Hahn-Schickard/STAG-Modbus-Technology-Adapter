#ifndef _MODBUS_TECHNOLOGY_ADAPTER_BUS_HPP
#define _MODBUS_TECHNOLOGY_ADAPTER_BUS_HPP

#include "Nonempty_Pointer/NonemptyPtr.hpp"
#include "Technology_Adapter_Interface/TechnologyAdapterInterface.hpp"
#include "Threadsafe_Containers/QueuedMutex.hpp"
#include "Threadsafe_Containers/SharedPtr.hpp"

#include "Config.hpp"
#include "ModbusTechnologyAdapterInterface.hpp"

namespace Technology_Adapter {
class ModbusTechnologyAdapter;
}

namespace Technology_Adapter::Modbus {

class Bus : public Threadsafe::EnableSharedFromThis<Bus> {
public:
  using NonemptyPtr = NonemptyPointer::NonemptyPtr<Threadsafe::SharedPtr<Bus>>;

  /// @throws `std::bad_alloc`.
  /// @throws `ModbusError`.
  /// @pre The lifetime of `*this` is included in the lifetime of `owner`
  Bus(ModbusTechnologyAdapterInterface& owner, Config::Bus::NonemptyPtr const&,
      LibModbus::Context::Factory const&, Config::Portname const&,
      Technology_Adapter::NonemptyDeviceRegistryPtr const&);
  ~Bus();

  /// @pre `connected`
  /// @throws `std::runtime_error`
  void buildModel(Information_Model::NonemptyDeviceBuilderInterfacePtr const&);
  void start(); /// @throws `std::runtime_error`
  void stop();

private:
  struct Connection {
    LibModbus::Context::Ptr context;
    bool connected = false;

    // Invariant: empty unless `connected`
    std::vector<ConstString::ConstString> devices_to_deregister;

    Connection(LibModbus::Context::Ptr&& context_)
        : context(std::move(context_)) {}
  };

  using ConnectionResource =
      Threadsafe::Resource<Connection, Threadsafe::QueuedMutex>;

  // This recursive method is local to `buildModel`.
  // @throws `std::bad_alloc`
  // @pre lifetime of `group` is contained in lifetime of `this`
  void buildGroup( //
      Information_Model::NonemptyDeviceBuilderInterfacePtr const&,
      std::string const&, // group id for `DeviceBuilderInterface`, "" for root
      NonemptyPtr const&, // `this`
      Config::Device::NonemptyPtr const&, //
      RegisterSet const& holding_registers, //
      RegisterSet const& input_registers, //
      Config::Group const&);

  void stop(ConnectionResource::ScopedAccessor&);

  // Called upon communication failure. Triggers re-detection for `actual_port_`
  // @throws `std::runtime_error` - always
  // @pre started
  [[noreturn]] void abort(ConnectionResource::ScopedAccessor&,
      ConstString::ConstString const& error_message);

  ModbusTechnologyAdapterInterface& owner_;
  Config::Bus::NonemptyPtr const config_;
  Config::Portname const actual_port_;
  NonemptyPointer::NonemptyPtr<HaSLI::LoggerPtr> const logger_;
  Technology_Adapter::NonemptyDeviceRegistryPtr const model_registry_;
  ConnectionResource connection_;

  friend class Readcallback;
};

} // namespace Technology_Adapter::Modbus

#endif // _MODBUS_TECHNOLOGY_ADAPTER_BUS_HPP
