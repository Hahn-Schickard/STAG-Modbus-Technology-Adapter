#ifndef _LIBMODBUS_ABSTRACTION_HPP
#define _LIBMODBUS_ABSTRACTION_HPP

#include "ThreadsafeStrerror.hpp"

/**
 * The purpose of this module is to provide some wrapping around libmodbus.
 * What it does:
 * - Encapsulate memory management.
 * - Hide C style global namespace entries, in particular macros.
 * - Convert `errno` use to exceptions (of type `ModbusError`).
 */

struct _modbus;
// For internal use only
// This is the one global namespace entry we cannot hide.

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

/// @brief Whenever the module throws, it throws this type.
struct ModbusError : public std::exception {
  int const errno_; /// either a POSIX error code or one of the below codes

  ModbusError() noexcept;
  char const* what() const noexcept override;

  /// Now follow error codes defined by libmodbus
  static int const MDATA;

private:
  Errno::ConstString const what_;
};

class Context {
public:
  Context() = delete;
  virtual ~Context();
  void connect(); /// may throw
  void close() noexcept;

  /// may throw with `errno == MDATA`
  int readRegisters(int addr, ReadableRegisterType, int nb, uint16_t* dest);

protected:
  _modbus* internal_;

  Context(_modbus* internal); /// may throw
};

class ContextRTU : public Context {
public:
  /// may throw with `errno == EINVAL` or `errno == ENOMEM`
  ContextRTU( //
      std::string const& device, int baud, char parity, //
      int data_bits, int stop_bits);

  /// may throw with `errno == EINVAL` or `errno == ENOMEM`
  ContextRTU( //
      std::string const& device, int baud, Parity, //
      int data_bits, int stop_bits);
  ~ContextRTU() override = default;
  void setSlave(int slave); // may throw with `errno == EINVAL``

private:
  /*
    As the libmodbus API does not specify the lifetime expectation of the
    `device` argument to `modbus_new_rtu`, we play it safe by keeping it
    alive here.
  */
  std::string device_;
};

} // namespace LibModbus

#endif //_LIBMODBUS_ABSTRACTION_HPP
