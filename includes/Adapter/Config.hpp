#ifndef _MODBUS_TECHNOLOGY_ADAPTER_CONFIG_HPP
#define _MODBUS_TECHNOLOGY_ADAPTER_CONFIG_HPP

#include <functional>
#include <string>

#include <Information_Model/DataVariant.hpp>
#include <Threadsafe_Containers/SharedPtr.hpp>

#include "LibmodbusAbstraction.hpp"
#include "RegisterSet.hpp"

namespace Technology_Adapter::Modbus::Config {

using Portname = std::string;

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
   * @brief Registers that permit read operation
   *
   * Must contain all registers used in `readables`, also in (transitive)
   * subgroups.
   * Burst optimization may utilize otherwise unused registers.
   */
  RegisterSet readable_registers;

  Device() = delete;
  Device(std::string /*id*/, std::string /*name*/, std::string /*description*/,
      int /*slave_id*/, size_t /*burst_size*/,
      std::vector<RegisterRange> const& /*readable*/);
};

struct Bus {
  using Ptr = Threadsafe::SharedPtr<Bus const>;

  std::vector<Portname> possible_serial_ports;
  int baud;
  LibModbus::Parity parity;
  int data_bits;
  int stop_bits;
  std::vector<Device> devices;

  Bus() = delete;
  Bus(std::vector<std::string> /*possible_serial_ports*/, int /*baud*/,
      LibModbus::Parity, int /*data_bits*/, int /*stop_bits*/);
};

} // namespace Technology_Adapter::Modbus::Config

#endif // _MODBUS_TECHNOLOGY_ADAPTER_CONFIG_HPP
