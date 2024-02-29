#include "internal/Modbus.hpp"

namespace Technology_Adapter::Modbus {

ModbusRTUContext::ModbusRTUContext(ConstString::ConstString const& port,
    Technology_Adapter::Modbus::Config::Bus const& bus)
    : libmodbus_context_(port, bus.baud, bus.parity, bus.data_bits,
        bus.stop_bits, bus.rts_delay) {}

void ModbusRTUContext::connect() { libmodbus_context_.connect(); }
void ModbusRTUContext::close() noexcept { libmodbus_context_.close(); }

void ModbusRTUContext::selectDevice(
    Technology_Adapter::Modbus::Config::Device const& device) {

  libmodbus_context_.selectDevice(device.slave_id);
}

int ModbusRTUContext::readRegisters(int addr,
    LibModbus::ReadableRegisterType register_type, int nb, uint16_t* dest) {

  return libmodbus_context_.readRegisters(addr, register_type, nb, dest);
}

ModbusRTUContext::Ptr ModbusRTUContext::make(
    ConstString::ConstString const& port,
    Technology_Adapter::Modbus::Config::Bus const& bus) {

  return std::make_shared<ModbusRTUContext>(port, bus);
}

} // namespace Technology_Adapter::Modbus
