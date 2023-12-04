#include "VirtualContext.hpp"

#include <random>

namespace ModbusTechnologyAdapterTests::Virtual_Context {

std::minstd_rand random; // NOLINT(cert-err58-cpp, cert-msc32-c, cert-msc51-cpp)
std::uniform_int_distribution<> noise{0, 1}; // NOLINT(cert-err58-cpp)

[[noreturn]] void throwModbus(int errnum) {
  errno = errnum;
  throw LibModbus::ModbusError();
}

VirtualContext::VirtualContext(
    // NOLINTNEXTLINE(modernize-pass-by-value)
    Technology_Adapter::Modbus::Config::Portname const& port,
    VirtualContextControl* control)
    : port_(port), control_(control) {}

void VirtualContext::connect() {
  if (control_->serial_port_exists) {
    connected_ = true;
  } else {
    throwModbus(ENOENT);
  }
}

void VirtualContext::close() noexcept { connected_ = false; }

void VirtualContext::selectDevice(
    Technology_Adapter::Modbus::Config::Device const& device) {

  selected_device_ = device.id;
}

int VirtualContext::readRegisters(
    int addr, LibModbus::ReadableRegisterType type, int nb, uint16_t* buffer) {

  auto device =
      control_->devices_.find(std::make_pair(port_, selected_device_));
  if (device == control_->devices_.end()) {
    // The selected device does not exist, so will not respond
    throwModbus(ETIMEDOUT);
  }

  if (type != device->second.register_type) {
    throwModbus(LibModbus::ModbusError::XILADD);
  }

  switch (device->second.quality) {
  case Quality::PERFECT:
    break;
  case Quality::UNRELIABLE:
    if (noise(random) == 1) {
      throwModbus(ETIMEDOUT);
    }
    break;
  case Quality::NOISY:
    if (noise(random) == 1) {
      throwModbus(LibModbus::ModbusError::BADCRC);
    }
    break;
  }

  // NOLINTBEGIN(readability-magic-numbers)
  switch (addr) {
  case 3:
  case 5:
    if (nb > 1) {
      throwModbus(LibModbus::ModbusError::MDATA);
    }
    break;
  case 2:
    if (nb > 2) {
      throwModbus(LibModbus::ModbusError::MDATA);
    }
    break;
  default:
    throwModbus(LibModbus::ModbusError::XILADD);
  }
  // NOLINTEND(readability-magic-numbers)

  for (int i = 0; i < nb; ++i) {
    buffer[i] = device->second.registers_value;
  }
  return nb;
}

LibModbus::Context::Factory VirtualContextControl::factory() {
  return //
      [this](ConstString::ConstString const& port,
          Technology_Adapter::Modbus::Config::Bus const&) {
        //
        return std::make_shared<VirtualContext>(port, this);
      };
}

void VirtualContextControl::setDevice( //
    Technology_Adapter::Modbus::Config::Portname const& port,
    ConstString::ConstString const& device_id,
    LibModbus::ReadableRegisterType register_type, uint16_t registers_value,
    Quality quality) {

  devices_.insert_or_assign(std::make_pair(port, device_id), //
      Behaviour{register_type, registers_value, quality});
}

void VirtualContextControl::reset() {
  serial_port_exists = true;
  devices_.clear();
}

} // namespace ModbusTechnologyAdapterTests::Virtual_Context
