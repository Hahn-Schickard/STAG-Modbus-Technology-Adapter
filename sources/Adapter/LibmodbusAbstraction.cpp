#include "LibmodbusAbstraction.hpp"
#include <cerrno>
#include <cstring>

namespace LibModbus {

char const* ModbusError::what() const noexcept { return "Who raised this?"; }

static int const MDATA = 1;

void Context::connect() {}
void Context::close() {}

int Context::readRegisters(int, int nb, uint16_t* dest) {
  for (size_t i = 0; i < nb; ++i) {
    dest[i] = std::rand();
  }
  return nb;
}

ContextRTU::ContextRTU(std::string const& device, int, char, int, int)
    : Context(), device_(device) {}

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

void ContextRTU::setSlave(int) {}

} // namespace LibModbus
