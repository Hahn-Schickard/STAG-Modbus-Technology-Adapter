#include "VirtualContext.hpp"

#include <random>

namespace ModbusTechnologyAdapterTests::VirtualContext {

std::minstd_rand random; // NOLINT(cert-msc32-c, cert-msc51-cpp)
std::uniform_int_distribution<> noise{0, 1};

[[noreturn]] void throwModbus(int errnum) {
  errno = errnum;
  throw LibModbus::ModbusError();
}

int fakeReadingRegisters(int addr, int nb) {
  switch (addr) {
  case 3:
  case 5:
    if (nb > 1) {
      throwModbus(LibModbus::ModbusError::MDATA);
    } else {
      return nb;
    }
  case 2:
    if (nb > 2) {
      throwModbus(LibModbus::ModbusError::MDATA);
    } else {
      return nb;
    }
  default:
    throwModbus(LibModbus::ModbusError::XILADD);
  }
}

void VirtualContext::connect() { connected_ = true; }
void VirtualContext::close() noexcept { connected_ = false; }

void VirtualContext::selectDevice(
    Technology_Adapter::Modbus::Config::Device const& device) {

  selected_device_ = device.id;
}

int VirtualContext::readRegisters(int addr,
    LibModbus::ReadableRegisterType type, int nb, uint16_t*) {

  if (selected_device_ == device1_name) {
    if (type != LibModbus::ReadableRegisterType::HoldingRegister) {
      throwModbus(LibModbus::ModbusError::XILADD);
    }

    return fakeReadingRegisters(addr, nb);

  } else if (selected_device_ == device2_name) {
    if (type != LibModbus::ReadableRegisterType::InputRegister) {
      throwModbus(LibModbus::ModbusError::XILADD);
    }
    if (noise(random) == 1) {
      throwModbus(ETIMEDOUT);
    }

    return fakeReadingRegisters(addr, nb);

  } else if (selected_device_ == device3_name) {
    if (type != LibModbus::ReadableRegisterType::InputRegister) {
      throwModbus(LibModbus::ModbusError::XILADD);
    }
    if (noise(random) == 1) {
      throwModbus(LibModbus::ModbusError::BADCRC);
    }

    return fakeReadingRegisters(addr, nb);

  } else {
    // The selected device does not exist, so will not respond
    throwModbus(ETIMEDOUT);
  }
}

VirtualContext::Ptr VirtualContext::make(
    ConstString::ConstString const& /*port*/,
    Technology_Adapter::Modbus::Config::Bus const&) {

  return std::make_shared<VirtualContext>();
}

} // namespace ModbusTechnologyAdapterTests::VirtualContext
