#include "LibmodbusAbstraction.hpp"
#include "modbus/modbus-rtu.h"
#include <cerrno>
#include <cstring>

namespace LibModbus {

ModbusError::ModbusError() : errno_(errno) { what_ = modbus_strerror(errno); }

char const* ModbusError::what() const noexcept { return what_.c_str(); }

static int const MDATA = EMBMDATA;

Context::Context(_modbus* internal) : internal_(internal) {
  if (internal == nullptr) {
    throw ModbusError();
  }
}

Context::~Context() { modbus_free(internal_); }

void Context::connect() {
  if (modbus_connect(internal_) != 0) {
    throw ModbusError();
  }
}

void Context::close() { modbus_close(internal_); }

int Context::readRegisters(
    int addr, ReadableRegisterType type, int nb, uint16_t* dest) {

  int retval = -1;
  switch (type) {
  case ReadableRegisterType::HoldingRegister:
    retval = modbus_read_registers(internal_, addr, nb, dest);
    break;
  case ReadableRegisterType::InputRegister:
    retval = modbus_read_input_registers(internal_, addr, nb, dest);
    break;
  }
  if (retval < 0) {
    throw ModbusError();
  }
  return retval;
}

ContextRTU::ContextRTU( //
    std::string const& device, int baud, char parity, //
    int data_bits, int stop_bits)
    : Context(
          modbus_new_rtu(device.c_str(), baud, parity, data_bits, stop_bits)),
      device_(device) {}

char charOfParity(Parity parity) {
  switch (parity) {
  case Parity::Even:
    return 'E';
  case Parity::Odd:
    return 'O';
  case Parity::None:
    return 'N';
  }
}

ContextRTU::ContextRTU( //
    std::string const& device, int baud, Parity parity, //
    int data_bits, int stop_bits)
    : ContextRTU(device, baud, charOfParity(parity), data_bits, stop_bits) {}

void ContextRTU::setSlave(int slave) {
  if (modbus_set_slave(internal_, slave) != 0) {
    throw ModbusError();
  }
}

} // namespace LibModbus
