#ifndef _MODBUS_TECHNOLOGY_ADAPTER_UNIT_TESTS_SPECS_HPP
#define _MODBUS_TECHNOLOGY_ADAPTER_UNIT_TESTS_SPECS_HPP

#include <Const_String/ConstString.hpp>

#include "Config.hpp"

namespace SpecsForTests {

using namespace Technology_Adapter::Modbus;

struct DeviceSpec {
  using Registers = std::vector<RegisterRange>;

  ConstString::ConstString id; // should be unique throughout a test
  int slave_id;
  Registers holding_registers;
  Registers input_registers;

  DeviceSpec() = delete;
  // NOLINTBEGIN(readability-identifier-naming)
  DeviceSpec(ConstString::ConstString id_, int slave_id_, //
      Registers holding_registers_, Registers input_registers_)
      : id(std::move(id_)), slave_id(slave_id_),
        holding_registers(std::move(holding_registers_)),
        input_registers(std::move(input_registers_)) {}
  // NOLINTEND(readability-identifier-naming)
};

Config::Device::NonemptyPtr specToConfig(DeviceSpec&&);

struct BusSpec {
  using Ports = std::vector<Config::Portname>;
  using Devices = std::vector<DeviceSpec>;

  Ports possible_ports;
  Devices devices;

  BusSpec() = delete;
  // NOLINTNEXTLINE(readability-identifier-naming)
  BusSpec(Ports&& possible_ports_, Devices&& devices_)
      : possible_ports(std::move(possible_ports_)),
        devices(std::move(devices_)) {}
};

Config::Bus::NonemptyPtr specToConfig(BusSpec&&);

} // namespace SpecsForTests

#endif // _MODBUS_TECHNOLOGY_ADAPTER_UNIT_TESTS_SPECS_HPP
