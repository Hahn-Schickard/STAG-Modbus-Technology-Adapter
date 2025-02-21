#ifndef _MODBUS_TECHNOLOGY_ADAPTER_CONFIG_HPP
#define _MODBUS_TECHNOLOGY_ADAPTER_CONFIG_HPP

/**
 * This module defines the data types for configuration of the Modbus TA.
 */

#include <functional>

#include <Const_String/ConstString.hpp>
#include <Information_Model/DataVariant.hpp>
#include <Nonempty/Pointer.hpp>
#include <Threadsafe_Containers/SharedPtr.hpp>

#include "LibmodbusAbstraction.hpp"
#include "RegisterSet.hpp"

namespace Technology_Adapter::Modbus::Config {

using Portname = ConstString::ConstString;

/**
 * @brief Represents a readable Modbus metric
 *
 * Contains the data necessary for an `Information_Model::Metric`
 * and for retrieving the respective value from Modbus registers.
 */
struct Readable {
  ConstString::ConstString const name;
  ConstString::ConstString const description;
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
};

/**
 * @brief An `Information_Model::DeviceElementGroup`  with `Readable` leaves
 */
struct Group {
  ConstString::ConstString const name;
  ConstString::ConstString const description;
  std::vector<Readable> const readables;
  std::vector<Group> const subgroups;

  Group() = delete;
};

/**
 * @brief Represents a Modbus slave as an `Information_Model::Device`
 */
struct Device : public Group {
  using NonemptyPtr = Nonempty::Pointer<Threadsafe::SharedPtr<Device const>>;

  /// In the sense of `Information_Model::NamedElement`
  ConstString::ConstString const id;

  int const slave_id;

  /// @brief Number of Modbus registers that may be read at once
  size_t const burst_size;

  /// @brief Number of retries on failures before giving up
  size_t const max_retries;

  /// @brief Delay before retries in ms
  size_t const retry_delay;

  /**
   * @brief Registers that permit operation `0x03` (read holding register)
   *
   * This and `input_registers` together must contain all registers used in
   * `readables`, also in (transitive) subgroups.
   * Burst optimization may utilize otherwise unused registers.
   */
  RegisterSet const holding_registers;

  /**
   * @brief Registers that permit operation `0x04` (read input register)
   *
   * This and `holding_registers` together must contain all registers used in
   * `readables`, also in (transitive) subgroups.
   * Burst optimization may utilize otherwise unused registers.
   */
  RegisterSet const input_registers;

  Device() = delete;
  Device(ConstString::ConstString id, ConstString::ConstString name,
      ConstString::ConstString description, //
      std::vector<Readable> readables, std::vector<Group> subgroups,
      int slave_id, size_t burst_size, size_t max_retries, size_t retry_delay,
      std::vector<RegisterRange> const& holding_registers,
      std::vector<RegisterRange> const& input_registers);
};

/**
 * @brief Represents a Modbus bus as a set of `Information_Model::Device`s
 */
struct Bus {
  using NonemptyPtr = Nonempty::Pointer<Threadsafe::SharedPtr<Bus const>>;

  std::vector<Portname> const possible_serial_ports;
  int const baud;
  LibModbus::Parity const parity;
  int const data_bits;
  int const stop_bits;

  /**
   * @brief Delay after setting RTS (if the bus has it)
   *
   * If the physical bus has an RTS line and it is supported by the driver,
   * this delay (in µs) is applied between toggling the RTS and sending the
   * message on the bus, and again between sending the message and toggling the
   * RTS back.
   * Otherwise it has no effect.
   */
  int rts_delay;

  /**
   * @brief Delay between successive bus uses during bus detection
   *
   * Bus uses are register read attempts. The delay is the min time (in µs)
   * between the end of one use and the start of the next use.
   */
  size_t inter_use_delay_when_searching;

  /**
   * @brief Delay between successive bus uses during normal operation
   *
   * Bus uses are register read attempts. The delay is the min time (in µs)
   * between the end of one use and the start of the next use.
   */
  size_t inter_use_delay_when_running;

  /**
   * @brief Delay between successive bus uses with different devices during bus
   * detection
   *
   * The delay (in µs) is applied, in addition to
   * `inter_use_delay_when_searching`, when switching between devices.
   */
  size_t inter_device_delay_when_searching;

  /**
   * @brief Delay between successive bus uses with different devices during
   * normal operation
   *
   * The delay (in µs) is applied, in addition to
   * `inter_use_delay_when_running`, when switching between devices.
   */
  size_t inter_device_delay_when_running;

  std::vector<Device::NonemptyPtr> const devices;

  /// @brief Composite of `devices`' IDs for the purpose of, e.g., logging
  ConstString::ConstString const id;

  Bus() = delete;
  Bus(std::vector<Portname> possible_serial_ports, int baud,
      LibModbus::Parity parity, int data_bits, int stop_bits, int rts_delay,
      size_t inter_use_delay_when_searching,
      size_t inter_use_delay_when_running,
      size_t inter_device_delay_when_searching,
      size_t inter_device_delay_when_running,
      std::vector<Device::NonemptyPtr> devices);
};

using Buses = std::vector<Bus::NonemptyPtr>;

} // namespace Technology_Adapter::Modbus::Config

#endif // _MODBUS_TECHNOLOGY_ADAPTER_CONFIG_HPP
