#ifndef _MODBUS_TECHNOLOGY_ADAPTER_UNIT_TESTS_VIRTUAL_CONTEXT_HPP
#define _MODBUS_TECHNOLOGY_ADAPTER_UNIT_TESTS_VIRTUAL_CONTEXT_HPP

#include <map>

#include "internal/Config.hpp"
#include "internal/LibmodbusAbstraction.hpp"

namespace ModbusTechnologyAdapterTests::Virtual_Context {

enum struct Quality {
  PERFECT,
  UNRELIABLE, // responds only with probability 1/2
  NOISY, // messes up CRC with probability 1/2
};

class VirtualContextControl;

// Hardcoded: All devices have registers 2, 3, and 5
class VirtualContext : public LibModbus::Context {
public:
  using Ptr = std::shared_ptr<VirtualContext>;

  VirtualContext() = delete;
  VirtualContext(VirtualContextControl* control);

  void connect() final;
  void close() noexcept final;
  void selectDevice(Technology_Adapter::Modbus::Config::Device const&) final;
  int readRegisters(
      int addr, LibModbus::ReadableRegisterType, int nb, uint16_t*) final;

private:
  bool connected_ = false;
  ConstString::ConstString selected_device_;
  VirtualContextControl* control_;
};

class VirtualContextControl {
public:
  bool serial_port_exists = true;

  LibModbus::Context::Factory factory();

  // Adds or replaces the specs for a device.
  // The specs will be used for all contexts emited from `Factory`s returned
  // by `factory` and for all registers of the device
  void setDevice( //
      ConstString::ConstString const& device_id,
      LibModbus::ReadableRegisterType, uint16_t registers_value, Quality);

  void reset(); // Removes all device specs

private:
  struct Behaviour {
    LibModbus::ReadableRegisterType register_type;
    uint16_t registers_value;
    Quality quality;
  };

  // indexed by device id
  std::map<ConstString::ConstString, Behaviour> devices_;

friend class VirtualContext;
};

} // namespace ModbusTechnologyAdapterTests::Virtual_Context

#endif // _MODBUS_TECHNOLOGY_ADAPTER_UNIT_TESTS_VIRTUAL_CONTEXT_HPP
