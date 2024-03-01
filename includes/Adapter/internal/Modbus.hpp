#ifndef _MODBUS_TECHNOLOGY_ADAPTER_MODBUS_HPP
#define _MODBUS_TECHNOLOGY_ADAPTER_MODBUS_HPP

#include <Const_String/ConstString.hpp>

#include "Config.hpp"

/**
 * @brief A further abstraction around the one from `LibModbusAbstraction.hpp`
 *
 * The present abstraction applies concepts from the present project, e.g.
 * `Config`.
 */
namespace Technology_Adapter::Modbus {

struct ModbusContext {
  enum struct Purpose {
    PortAutoDetection,
    NormalOperation,
  };

  using Ptr = std::shared_ptr<ModbusContext>;

  using Factory = std::function<Ptr(
      ConstString::ConstString const& port, Config::Bus const&, Purpose)>;

  virtual ~ModbusContext() = default;
  virtual void connect() = 0; /// @throws `ModbusError`
  virtual void close() noexcept = 0;

  /// @throws `ModbusError`
  virtual void selectDevice(Config::Device const&) = 0;

  /**
   * Reads up to `nb` registers starting at address `addr` and stores their
   * values in the buffer `dest`.
   *
   * @returns the number of registers actually read
   * @throws `ModbusError`
   * @pre connected
   */
  virtual int readRegisters(
      int addr, LibModbus::ReadableRegisterType, int nb, uint16_t* dest) = 0;
};

class ModbusRTUContext : public ModbusContext {
public:
  using Ptr = std::shared_ptr<ModbusRTUContext>;
  ModbusRTUContext(
      ConstString::ConstString const& port, Config::Bus const&, Purpose);

  virtual void connect() override; /// @throws `ModbusError`
  virtual void close() noexcept override;
  virtual void selectDevice(
      Config::Device const&) override; /// @throws `ModbusError`
  virtual int readRegisters(int addr, LibModbus::ReadableRegisterType, int nb,
      uint16_t* dest) override; /// @throws `ModbusError`

  /// @brief A `Factory`
  /// @throws `ModbusError`
  static Ptr make(
      ConstString::ConstString const& port, Config::Bus const&, Purpose);

private:
  LibModbus::ContextRTU libmodbus_context_;
  std::chrono::microseconds inter_use_delay_;
  std::chrono::microseconds inter_device_delay_;
  std::chrono::time_point<std::chrono::steady_clock> end_of_last_use_;
  int last_use_slave_id_ = -1;
  int current_slave_id_;
};

} // namespace Technology_Adapter::Modbus

#endif // _MODBUS_TECHNOLOGY_ADAPTER_MODBUS_HPP
