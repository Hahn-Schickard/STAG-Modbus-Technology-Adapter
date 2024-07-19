#ifndef _MODBUS_TECHNOLOGY_ADAPTER_BUS_HPP
#define _MODBUS_TECHNOLOGY_ADAPTER_BUS_HPP

#include "Nonempty_Pointer/NonemptyPtr.hpp"
#include "Technology_Adapter_Interface/TechnologyAdapterInterface.hpp"
#include "Threadsafe_Containers/QueuedMutex.hpp"
#include "Threadsafe_Containers/SharedPtr.hpp"

#include "Config.hpp"
#include "Modbus.hpp"
#include "ModbusTechnologyAdapterInterface.hpp"

namespace Technology_Adapter {
class ModbusTechnologyAdapter;
}

namespace Technology_Adapter::Modbus {

/**
 * @brief A Modbus bus
 *
 * Actual access to the bus is not visible in this API. It happens through
 * callbacks that are created in `buildModel` and handed to the registry.
 *
 * Below, we use `connected` as a shorthand for the `connected` member of the
 * value of the `connection_` `Resource`.
 */
class Bus : public Threadsafe::EnableSharedFromThis<Bus> {
public:
  using NonemptyPtr = NonemptyPointer::NonemptyPtr<Threadsafe::SharedPtr<Bus>>;

  /**
   * During the lifetime of `this`, `owner->cancelBus` is called at most once
   * from this `Bus`; namely following a change of `connected` from `true` to
   * `false` except from `stop`.
   *
   * @throws `std::bad_alloc`.
   * @throws `ModbusError`.
   * @pre The lifetime of `*this` is included in the lifetime of `owner`
   * @post `!connected`
   */
  Bus(ModbusTechnologyAdapterInterface& owner, Config::Bus::NonemptyPtr const&,
      ModbusContext::Factory const&, Config::Portname const&,
      Technology_Adapter::NonemptyDeviceRegistryPtr const&);

  ~Bus();

  /**
   * @brief Establishes a connection and registers all devices
   *
   * @pre `!connected`
   * @post `connected`
   * @throws `std::runtime_error`
   */
  void start(Information_Model::NonemptyDeviceBuilderInterfacePtr const&);

  /**
   * @brief Closes the connection and deregisters all devices
   *
   * @pre `connected`
   * @post `!connected`
   */
  void stop();

private:
  struct Connection {
    ModbusContext::Ptr context;
    bool connected = false;

    // Invariant: empty unless `connected`
    std::vector<ConstString::ConstString> devices_to_deregister;

    Connection(ModbusContext::Ptr&& context_) : context(std::move(context_)) {}
  };

  using ConnectionResource =
      Threadsafe::PrivateResource<Connection, Threadsafe::QueuedMutex>;

  // Registers all devices
  // This method is local to `start`
  // @pre `connected`
  // @throws `std::runtime_error`
  void buildModel(Information_Model::NonemptyDeviceBuilderInterfacePtr const&);

  // This recursive method is local to `buildModel`.
  // @throws `std::bad_alloc`
  // @throws `std::runtime_error`
  // @pre lifetime of `group` is contained in lifetime of `this`
  void buildGroup( //
      Information_Model::NonemptyDeviceBuilderInterfacePtr const&,
      std::string const& group_id, // for `DeviceBuilderInterface`, "" for root
      NonemptyPtr const& shared_this,
      Config::Device::NonemptyPtr const&, //
      RegisterSet const& holding_registers, //
      RegisterSet const& input_registers, //
      Config::Group const&);

  /*
    - Closes the connection
    - Deregisters all devices
  */
  void stop(ConnectionResource::ScopedAccessor&);

  /*
    Called upon communication failure.
    - Closes the connection
    - Deregisters all devices
    - Triggers re-detection for `actual_port_`
    @throws `std::runtime_error` - always
    @pre `connected`
  */
  [[noreturn]] void abort(ConnectionResource::ScopedAccessor&,
      ConstString::ConstString const& error_message);

  ModbusTechnologyAdapterInterface& owner_;
  Config::Bus::NonemptyPtr const config_;
  Config::Portname const actual_port_;
  NonemptyPointer::NonemptyPtr<HaSLI::LoggerPtr> const logger_;
  Technology_Adapter::NonemptyDeviceRegistryPtr const model_registry_;
  ConnectionResource connection_;

  friend struct Readcallback;
};

} // namespace Technology_Adapter::Modbus

#endif // _MODBUS_TECHNOLOGY_ADAPTER_BUS_HPP
