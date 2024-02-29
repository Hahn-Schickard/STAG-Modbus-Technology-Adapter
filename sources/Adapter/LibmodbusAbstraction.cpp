#include "internal/LibmodbusAbstraction.hpp"

#include <cerrno>
#include <cstring>
#include <thread>

#include "modbus/modbus-rtu.h"

#include "internal/Config.hpp"

namespace LibModbus {

// ModbusError

ModbusError::ModbusError() noexcept
    : errno_(errno), what_(Errno::generic_strerror(modbus_strerror, errno_)) {}

char const* ModbusError::what() const noexcept { return what_.c_str(); }

bool ModbusError::retryFeasible() const {
  if ((errno_ == XSBUSY) || (errno_ == XMEMPAR) || (errno_ == BADCRC) ||
      (errno_ == ETIMEDOUT)) {

    return true; // NOLINT(readability-simplify-boolean-expr)
  } else {
    /*
      In particular:
      - from libmodbus:
        - XILFUN, XILADD, XILVAL, XSFAIL, XACK, XGPATH, XGTAR
        - XNACK seems to be deprecated
        - BADDATA, BADEXC, UNKEXC, MDATA, BADSLAVE
      - POSIX codes that were witnessed:
        - ENOENT
    */
    return false;
  }
}

// NOLINTBEGIN(readability-identifier-naming)
int const ModbusError::XILFUN = EMBXILFUN;
int const ModbusError::XILADD = EMBXILADD;
int const ModbusError::XILVAL = EMBXILVAL;
int const ModbusError::XSFAIL = EMBXSFAIL;
int const ModbusError::XACK = EMBXACK;
int const ModbusError::XSBUSY = EMBXSBUSY;
int const ModbusError::XNACK = EMBXNACK;
int const ModbusError::XMEMPAR = EMBXMEMPAR;
int const ModbusError::XGPATH = EMBXGPATH;
int const ModbusError::XGTAR = EMBXGTAR;
int const ModbusError::BADCRC = EMBBADCRC;
int const ModbusError::BADDATA = EMBBADDATA;
int const ModbusError::BADEXC = EMBBADEXC;
int const ModbusError::UNKEXC = EMBUNKEXC;
int const ModbusError::MDATA = EMBMDATA;
int const ModbusError::BADSLAVE = EMBBADSLAVE;
// NOLINTEND(readability-identifier-naming)

// LibModbusContext

LibModbusContext::LibModbusContext(_modbus* internal) : internal_(internal) {
  if (internal == nullptr) {
    throw ModbusError();
  }
}

LibModbusContext::~LibModbusContext() { modbus_free(internal_); }

void LibModbusContext::connect() {
  if (modbus_connect(internal_) != 0) {
    throw ModbusError();
  }
}

void LibModbusContext::close() noexcept { modbus_close(internal_); }

int LibModbusContext::readRegisters(
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

// ContextRTU

// Converts `parity` into the `char` expected by the libmodbus API
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

ContextRTU::ContextRTU(ConstString::ConstString const& port, int baud,
    Parity parity, int data_bits, int stop_bits, int rts_delay)
    : LibModbusContext(modbus_new_rtu(port.c_str(), baud, charOfParity(parity),
          data_bits, stop_bits)),
      device_(port) {

  if (rts_delay > 0) {
    modbus_rtu_set_rts_delay(internal_, rts_delay);
  }
}

void ContextRTU::selectDevice(int slave_id) {
  if (modbus_set_slave(internal_, slave_id) != 0) {
    throw ModbusError();
  }
}

} // namespace LibModbus
