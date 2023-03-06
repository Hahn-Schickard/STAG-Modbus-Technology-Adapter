#include "LibmodbusAbstraction.hpp"
#include "modbus/modbus-rtu.h"
#include <cerrno>
#include <cstring>

namespace LibModbus {

ModbusError::ModbusError() : errno_(errno) {
  what_ = modbus_strerror(errno);
}

char const* ModbusError::what() const noexcept { return what_.c_str(); }

static int const MDATA = EMBMDATA;

Context::Context(_modbus* internal) : internal_(internal) {
  if (!internal)
    throw ModbusError();
}

Context::~Context() { modbus_free(internal_); }

void Context::connect() {
  if (modbus_connect(internal_))
    throw ModbusError();
}

void Context::close() { modbus_close(internal_); }

int Context::readRegisters(int addr, int nb, uint16_t* dest) {
  int retval = modbus_read_registers(internal_, addr, nb, dest);
  if (retval < 0)
    throw ModbusError();
  return retval;
}


ContextRTU::ContextRTU(
    std::string device, int baud, char parity, int data_bits, int stop_bits)
    : Context(
          modbus_new_rtu(device.c_str(), baud, parity, data_bits, stop_bits)),
      device_(device) {}

void ContextRTU::setSlave(int slave) {
  if (modbus_set_slave(internal_, slave))
    throw ModbusError();
}

} // namespace LibModbus
