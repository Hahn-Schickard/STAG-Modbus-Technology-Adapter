#include "internal/Modbus.hpp"

#include <thread>

namespace Technology_Adapter::Modbus {

namespace {
size_t interUseDelay(Config::Bus const& bus, ModbusContext::Purpose purpose) {
  switch (purpose) {
  case ModbusContext::Purpose::PortAutoDetection:
    return bus.inter_use_delay_when_searching;
  case ModbusContext::Purpose::NormalOperation:
    return bus.inter_use_delay_when_running;
  }
}

size_t interDeviceDelay(Config::Bus const& bus, ModbusContext::Purpose purpose) {
  switch (purpose) {
  case ModbusContext::Purpose::PortAutoDetection:
    return bus.inter_device_delay_when_searching;
  case ModbusContext::Purpose::NormalOperation:
    return bus.inter_device_delay_when_running;
  }
}
} // namespace

ModbusRTUContext::ModbusRTUContext(ConstString::ConstString const& port,
    Config::Bus const& bus, Purpose purpose)
    : libmodbus_context_(port, bus.baud, bus.parity, bus.data_bits,
          bus.stop_bits, bus.rts_delay),
      end_of_last_use_(std::chrono::steady_clock::now()) {

  inter_use_delay_ = std::chrono::microseconds(interUseDelay(bus, purpose));
  inter_device_delay_ =
      std::chrono::microseconds(interDeviceDelay(bus, purpose));
  inter_device_delay_ += inter_use_delay_;
}

void ModbusRTUContext::connect() { libmodbus_context_.connect(); }
void ModbusRTUContext::close() noexcept { libmodbus_context_.close(); }

void ModbusRTUContext::selectDevice(
    Technology_Adapter::Modbus::Config::Device const& device) {

  libmodbus_context_.selectDevice(device.slave_id);
  current_slave_id_ = device.slave_id;
}

int ModbusRTUContext::readRegisters(int addr,
    LibModbus::ReadableRegisterType register_type, int nb, uint16_t* dest) {

  auto required_delay = current_slave_id_ == last_use_slave_id_
      ? inter_use_delay_
      : inter_device_delay_;
  last_use_slave_id_ = current_slave_id_;
  auto elapsed_since_last_use =
      std::chrono::steady_clock::now() - end_of_last_use_;
  if (elapsed_since_last_use < required_delay) {
    std::this_thread::sleep_for(required_delay - elapsed_since_last_use);
  }

  auto retval = libmodbus_context_.readRegisters(addr, register_type, nb, dest);

  end_of_last_use_ = std::chrono::steady_clock::now();
  return retval;
}

ModbusRTUContext::Ptr ModbusRTUContext::make(
    ConstString::ConstString const& port,
    Technology_Adapter::Modbus::Config::Bus const& bus, Purpose purpose) {

  return std::make_shared<ModbusRTUContext>(port, bus, purpose);
}

} // namespace Technology_Adapter::Modbus
