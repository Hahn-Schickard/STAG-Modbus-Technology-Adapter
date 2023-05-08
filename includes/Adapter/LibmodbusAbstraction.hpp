#ifndef _LIBMODBUS_ABSTRACTION_HPP
#define _LIBMODBUS_ABSTRACTION_HPP

#include <string>

/**
 * The purpose of this module is to provide some wrapping around libmodbus.
 * What it does:
 * - Encapsulate memory management.
 * - Hide C style global namespace entries, in particular macros.
 * - Convert `errno` use to exceptions (of type `ModbusError`).
 */

namespace LibModbus {

enum struct Parity {
  Even,
  Odd,
  None,
};

/// Whenever the module throws (in this branch it doesn't), it throws this type.
struct ModbusError : public std::exception {
  int errno_ = 0; /// either a POSIX error code or one of the below codes
  ModbusError() = default;
  char const* what() const noexcept override;

  /// Now follow error codes defined by libmodbus
  static int const MDATA;
};

class Context {
public:
  virtual ~Context() = default;
  void connect(); /// may throw
  void close();

  /// may throw with `errno==MDATA`
  int readRegisters(int addr, int nb, uint16_t* dest);

protected:
  Context() = default;
};

class ContextRTU : public Context {
public:
  /// may throw with `errno==EINVAL` or `errno==ENOMEM`
  ContextRTU( //
      std::string const& device, int baud, char parity, //
      int data_bits, int stop_bits);
  ContextRTU( //
      std::string const& device, int baud, Parity, //
      int data_bits, int stop_bits);
  ~ContextRTU() override = default;
  void setSlave(int slave); // may throw with `errno==EINVAL``

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
