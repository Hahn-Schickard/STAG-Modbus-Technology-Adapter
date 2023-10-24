#ifndef _MODBUS_TECHNOLOGY_ADAPTER_UNIT_TESTS_VIRTUAL_CONTEXT_HPP
#define _MODBUS_TECHNOLOGY_ADAPTER_UNIT_TESTS_VIRTUAL_CONTEXT_HPP

#include "internal/Config.hpp"
#include "internal/LibmodbusAbstraction.hpp"

namespace ModbusTechnologyAdapterTests::VirtualContext {

static ConstString::ConstString device1_name{"Device 1"};
static ConstString::ConstString device2_name{"Device 2"};
static ConstString::ConstString device3_name{"Device 3"};

/*
  Hardcoded behaviour:
  - Device 1 has holding registers 2,3,5
  - Device 2 has input registers 2,3,5 and responds only with probability 1/2
  - Device 3 has input registers 2,3,5 and messes up CRC with probability 1/2
*/
class VirtualContext : public LibModbus::Context {
public:
  using Ptr = std::shared_ptr<VirtualContext>;

  void connect() final;
  void close() noexcept final;
  void selectDevice(Technology_Adapter::Modbus::Config::Device const&) final;

  int readRegisters(int addr, LibModbus::ReadableRegisterType, int nb,
      uint16_t*) final;

  // a `Factory`
  static Ptr make(ConstString::ConstString const& port,
      Technology_Adapter::Modbus::Config::Bus const&);

private:
  bool connected_ = false;
  ConstString::ConstString selected_device_;
};

} // namespace ModbusTechnologyAdapterTests::VirtualContext

#endif // _MODBUS_TECHNOLOGY_ADAPTER_UNIT_TESTS_VIRTUAL_CONTEXT_HPP
