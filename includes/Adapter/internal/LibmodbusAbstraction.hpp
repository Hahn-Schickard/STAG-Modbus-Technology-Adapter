#ifndef _LIBMODBUS_ABSTRACTION_HPP
#define _LIBMODBUS_ABSTRACTION_HPP

#include <Const_String/ConstString.hpp>

#include "ThreadsafeStrerror.hpp"

/**
 * The purpose of this module is to provide some wrapping around libmodbus.
 * What it does:
 * - Encapsulate memory management.
 * - Hide C style global namespace entries, in particular macros.
 * - Convert `errno` use to exceptions (of type `ModbusError`).
 */

// For internal use only
// This is the one global namespace entry we cannot hide.
struct _modbus;

namespace Technology_Adapter::Modbus::Config {
struct Bus;
struct Device;
} // namespace Technology_Adapter::Modbus::Config

namespace LibModbus {

enum struct Parity {
  Even,
  Odd,
  None,
};

enum struct ReadableRegisterType {
  HoldingRegister,
  InputRegister,
};

/**
 * @brief Whenever the module throws, it throws this type
 *
 * Included is an error code as used by `libmodbus`. Unfortunately, the
 * `libmodbus` documentation is not reliable w.r.t. which functions may emit
 * which codes. Hence we are not in a position to narrow anthing down.
 */
struct ModbusError : public std::exception {
  // `errno` is a macro, hence the additional underscore
  int const errno_; /// either a POSIX error code or one of the below codes

  ModbusError() noexcept;
  char const* what() const noexcept override;

  /**
   * @brief Given `errno_`, does it make sense to retry the throwing operation?
   *
   * If in doubt, returns `false`
   */
  bool retryFeasible() const;

  /// Now follow error codes defined by libmodbus
  static int const XILFUN;
  static int const XILADD;
  static int const XILVAL;
  static int const XSFAIL;
  static int const XACK;
  static int const XSBUSY;
  static int const XNACK;
  static int const XMEMPAR;
  static int const XGPATH;
  static int const XGTAR;
  static int const BADCRC;
  static int const BADDATA;
  static int const BADEXC;
  static int const UNKEXC;
  static int const MDATA;
  static int const BADSLAVE;

private:
  ConstString::ConstString const what_;
};

/// @brief Abstract class for communication with a Modbus
struct Context {
  using Ptr = std::shared_ptr<Context>;
  using Factory = std::function<Ptr(ConstString::ConstString const& port,
      Technology_Adapter::Modbus::Config::Bus const&)>;

  virtual ~Context() = default;
  virtual void connect() = 0; /// @throws `ModbusError`
  virtual void close() noexcept = 0;

  /// @throws `ModbusError`
  virtual void selectDevice(
      Technology_Adapter::Modbus::Config::Device const&) = 0;

  /**
   * Reads up to `nb` registers starting at address `addr` and stores their
   * values in the buffer `dest`.
   *
   * @returns the number of registers actually read
   * @throws `ModbusError`
   * @pre connected
   */
  virtual int readRegisters(
      int addr, ReadableRegisterType, int nb, uint16_t* dest) = 0;
};

/*
  @brief A specialization of `Context` based on `libmodbus`

  This class is still abstract and for internal use only.
*/
class LibModbusContext : public Context {
public:
  LibModbusContext() = delete;
  ~LibModbusContext() override;
  void connect() override;
  void close() noexcept override;
  int readRegisters(
      int addr, ReadableRegisterType, int nb, uint16_t* dest) override;

protected:
  _modbus* internal_;

  LibModbusContext(_modbus* internal); // @throws `ModbusError`
};

/// @brief An implementation of `Context` for Modbus RTU based on libmodbus
class ContextRTU : public LibModbusContext {
public:
  using Ptr = std::shared_ptr<ContextRTU>;

  ~ContextRTU() override = default;

  ContextRTU(ConstString::ConstString const& port,
      Technology_Adapter::Modbus::Config::Bus const&);

  /// @throws `ModbusError`
  void selectDevice(Technology_Adapter::Modbus::Config::Device const&) override;

  /// @brief A `Factory`
  /// @throws `ModbusError`
  static Ptr make(ConstString::ConstString const& port,
      Technology_Adapter::Modbus::Config::Bus const&);

private:
  /*
    As the libmodbus API does not specify the lifetime expectation of the
    `device` argument to `modbus_new_rtu`, we play it safe by keeping it
    alive here.
  */
  ConstString::ConstString device_;
};

} // namespace LibModbus

#endif //_LIBMODBUS_ABSTRACTION_HPP
