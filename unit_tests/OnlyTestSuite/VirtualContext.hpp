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

// Hardcoded: All devices have registers 2, 3, and 5
class VirtualContext : public LibModbus::Context {
public:
  using Ptr = std::shared_ptr<VirtualContext>;

  void connect() final;
  void close() noexcept final;
  void selectDevice(Technology_Adapter::Modbus::Config::Device const&) final;
  int readRegisters(
      int addr, LibModbus::ReadableRegisterType, int nb, uint16_t*) final;

  // Adds or replaces the specs for a device.
  // The specs will be used for all contexts and for all registers of the device
  static void setDevice( //
      ConstString::ConstString const& device_id,
      LibModbus::ReadableRegisterType, uint16_t registers_value, Quality);

  static void reset(); // Removes all device specs

  // a `Factory`
  static Ptr make(ConstString::ConstString const& port,
      Technology_Adapter::Modbus::Config::Bus const&);

private:
  struct Behaviour {
    LibModbus::ReadableRegisterType register_type;
    uint16_t registers_value;
    Quality quality;
  };

  // indexed by device id
  static std::map<ConstString::ConstString, Behaviour> devices_;

  bool connected_ = false;
  ConstString::ConstString selected_device_;
};

} // namespace ModbusTechnologyAdapterTests::Virtual_Context

#endif // _MODBUS_TECHNOLOGY_ADAPTER_UNIT_TESTS_VIRTUAL_CONTEXT_HPP
