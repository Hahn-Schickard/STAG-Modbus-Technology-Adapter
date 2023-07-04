#ifndef _MODBUS_TECHNOLOGY_ADAPTER_CONFIG_HPP
#define _MODBUS_TECHNOLOGY_ADAPTER_CONFIG_HPP

#include <functional>
#include <string>

#include <Information_Model/DataVariant.hpp>

#include "LibmodbusAbstraction.hpp"
#include "RegisterSet.hpp"

namespace Technology_Adapter::Modbus::Config {

struct Readable {
  std::string name;
  std::string description;
  Information_Model::DataType type;

  /// Indices of the Modbus register to decode from
  std::vector<int> registers;

  /**
   * @pre The `vector` has the same length as `registers`
   * @post The `DataVariant` conforms to `type`
   */
  using Decoder = std::function<Information_Model::DataVariant(
      std::vector<uint16_t> const&)>;

  Decoder decode;

  Readable() = delete;
  Readable(std::string /*name*/, std::string /*description*/,
      Information_Model::DataType, std::vector<int>, Decoder);
};

struct Group {
  std::string name;
  std::string description;
  std::vector<Readable> readables;
  std::vector<Group> subgroups;

  Group() = delete;
  Group(std::string /*name*/, std::string /*description*/);
};

struct Device : public Group {
  std::string id; /// In the sense of `Information_Model::NamedElement`
  int slave_id;
  size_t burst_size; /// Number of Modbus registers that may be read at once

  /**
   * @brief Registers that permit operation 0x03 (read holding register)
   *
   * This and `input_registers` together must contain all registers used in
   * `readables`, also in (transitive) subgroups.
   * Burst optimization may utilize otherwise unused registers.
   */
  std::vector<RegisterRange> holding_registers;

  /**
   * @brief Registers that permit operation 0x04 (read input register)
   *
   * This and `holding_registers` together must contain all registers used in
   * `readables`, also in (transitive) subgroups.
   * Burst optimization may utilize otherwise unused registers.
   */
  std::vector<RegisterRange> input_registers;

  Device() = delete;
  Device(std::string /*id*/, std::string /*name*/, std::string /*description*/,
      int /*slave_id*/, size_t /*burst_size*/,
      std::vector<RegisterRange> /*holding_registers*/,
      std::vector<RegisterRange> /*input_registers*/);
};

struct Bus {
  std::string serial_port;
  int baud;
  LibModbus::Parity parity;
  int data_bits;
  int stop_bits;
  std::vector<Device> devices;

  Bus() = delete;
  Bus(std::string /*serial_port*/, int /*baud*/, LibModbus::Parity,
      int /*data_bits*/, int /*stop_bits*/);
};

} // namespace Technology_Adapter::Modbus::Config

#endif // _MODBUS_TECHNOLOGY_ADAPTER_CONFIG_HPP
