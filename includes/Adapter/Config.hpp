#ifndef _MODBUS_TECHNOLOGY_ADAPTER_CONFIG_HPP
#define _MODBUS_TECHNOLOGY_ADAPTER_CONFIG_HPP

#include <functional>
#include <string>

#include <Information_Model/DataVariant.hpp>

#include "LibmodbusAbstraction.hpp"

namespace Modbus_Technology_Adapter::Config {

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

  Readable(std::string /*name*/, std::string /*description*/,
      Information_Model::DataType, std::vector<int>, Decoder);
};

struct Group {
  std::string name;
  std::string description;
  std::vector<Readable> readables;
  std::vector<Group> subgroups;

  Group(std::string /*name*/, std::string /*description*/);
};

struct Device : public Group {
  std::string id; /// In the sense of `Information_Model::NamedElement`
  int slave_id;
  size_t burst_size; /// Number of Modbus registers that may be read at once

  Device(std::string /*id*/, std::string /*name*/, std::string /*description*/,
      int /*slave_id*/, size_t /*burst_size*/);
};

struct Bus {
  std::string serial_port;
  int baud;
  LibModbus::Parity parity;
  int data_bits;
  int stop_bits;
  std::vector<Device> devices;

  Bus(std::string /*serial_port*/, int /*baud*/, LibModbus::Parity,
      int /*data_bits*/, int /*stop_bits*/);
};

} // namespace Modbus_Technology_Adapter::Config

#endif // _MODBUS_TECHNOLOGY_ADAPTER_CONFIG_HPP
