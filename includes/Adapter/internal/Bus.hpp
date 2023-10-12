#ifndef _MODBUS_TECHNOLOGY_ADAPTER_BUS_HPP
#define _MODBUS_TECHNOLOGY_ADAPTER_BUS_HPP

#include "Nonempty_Pointer/NonemptyPtr.hpp"
#include "Technology_Adapter_Interface/TechnologyAdapterInterface.hpp"
#include "Threadsafe_Containers/QueuedMutex.hpp"
#include "Threadsafe_Containers/SharedPtr.hpp"

#include "../Config.hpp"

namespace Technology_Adapter {
class ModbusTechnologyAdapter;
}

namespace Technology_Adapter::Modbus {

class Bus : public Threadsafe::EnableSharedFromThis<Bus> {
public:
  using NonemptyPtr = NonemptyPointer::NonemptyPtr<Threadsafe::SharedPtr<Bus>>;

  /// @throws `std::bad_alloc`.
  /// @throws `ModbusError`.
  Bus(ModbusTechnologyAdapter&, Config::Bus const&, Config::Portname const&,
    Technology_Adapter::NonemptyDeviceRegistryPtr const&);
  ~Bus();

  /// @pre not connected
  /// @throws `std::runtime_error`
  void buildModel(Information_Model::NonemptyDeviceBuilderInterfacePtr const&);
  void start(); /// @throws `ModbusError`
  void stop();

private:
  struct Connection {
    LibModbus::ContextRTU context;
    bool connected = false;
    std::vector<std::string> registered_devices; // `const` while `connected`

    template <class... Args> // constructor args for `ContextRTU`
    Connection(Args&&... args) : context(std::forward<Args>(args)...) {}
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
      Config::Device const&, //
      RegisterSet const& holding_registers, //
      RegisterSet const& input_registers, //
      Config::Group const&);

  void stop(ConnectionResource::ScopedAccessor&);

  // @throws `std::runtime_error` - always
  // @pre started
  void abort(ConnectionResource::ScopedAccessor&, std::string error_message);

  ModbusTechnologyAdapter& owner_;
  Config::Bus const config_;
  Config::Portname const actual_port_;
  NonemptyPointer::NonemptyPtr<HaSLI::LoggerPtr> const logger_;
  Technology_Adapter::NonemptyDeviceRegistryPtr const model_registry_;
  ConnectionResource connection_;

  friend class Readcallback;
};

} // namespace Technology_Adapter::Modbus

#endif // _MODBUS_TECHNOLOGY_ADAPTER_BUS_HPP
