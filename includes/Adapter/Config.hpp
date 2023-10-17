#ifndef _MODBUS_TECHNOLOGY_ADAPTER_CONFIG_HPP
#define _MODBUS_TECHNOLOGY_ADAPTER_CONFIG_HPP

#include <functional>
#include <string>

#include <Information_Model/DataVariant.hpp>
#include <Nonempty_Pointer/NonemptyPtr.hpp>
#include <Threadsafe_Containers/SharedPtr.hpp>

#include "LibmodbusAbstraction.hpp"
#include "RegisterSet.hpp"

namespace Technology_Adapter::Modbus::Config {

using Portname = std::string;

struct Readable {
  std::string const name;
  std::string const description;
  Information_Model::DataType const type;

  /// Indices of the Modbus register to decode from
  std::vector<int> const registers;

  /**
   * @pre The `vector` has the same length as `registers`
   * @post The `DataVariant` conforms to `type`
   */
  using Decoder = std::function<Information_Model::DataVariant(
      std::vector<uint16_t> const&)>;

  Decoder const decode;

  Readable() = delete;
  Readable(std::string name, std::string description,
      Information_Model::DataType type, std::vector<int> registers,
      Decoder decode);
};

struct Group {
  std::string const name;
  std::string const description;
  std::vector<Readable> const readables;
  std::vector<Group> const subgroups;

  Group() = delete;
  Group(std::string name, std::string description,
      std::vector<Readable> readables, std::vector<Group> subgroups);
};

struct Device : public Group {
  std::string const id; /// In the sense of `Information_Model::NamedElement`
  int const slave_id;

  /// @brief Number of Modbus registers that may be read at once
  size_t const burst_size;

  /// @brief Number of retries on failures before giving up
  size_t const max_retries;

  /// @brief Delay before retries in ms
  size_t const retry_delay;

  /**
   * @brief Registers that permit operation 0x03 (read holding register)
   *
   * This and `input_registers` together must contain all registers used in
   * `readables`, also in (transitive) subgroups.
   * Burst optimization may utilize otherwise unused registers.
   */
  RegisterSet const holding_registers;

  /**
   * @brief Registers that permit operation 0x04 (read input register)
   *
   * This and `holding_registers` together must contain all registers used in
   * `readables`, also in (transitive) subgroups.
   * Burst optimization may utilize otherwise unused registers.
   */
  RegisterSet const input_registers;

  Device() = delete;
  Device(std::string id, std::string name, std::string description,
      std::vector<Readable> readables, std::vector<Group> subgroups,
      int slave_id, size_t burst_size, size_t max_retries, size_t retry_delay,
      std::vector<RegisterRange> const& holding_registers,
      std::vector<RegisterRange> const& input_registers);
};

struct Bus {
  using NonemptyPtr =
      NonemptyPointer::NonemptyPtr<Threadsafe::SharedPtr<Bus const>>;

  std::vector<Portname> const possible_serial_ports;
  int const baud;
  LibModbus::Parity const parity;
  int const data_bits;
  int const stop_bits;
  std::vector<Device> const devices;

  /// @brief Composite of `devices`' IDs for the purpose of, e.g., logging
  std::string const id;

  Bus() = delete;
  Bus(std::vector<std::string> possible_serial_ports, int baud,
      LibModbus::Parity parity, int data_bits, int stop_bits,
      std::vector<Device> devices);
};

using Buses = std::vector<Bus>;

} // namespace Technology_Adapter::Modbus::Config

#endif // _MODBUS_TECHNOLOGY_ADAPTER_CONFIG_HPP
