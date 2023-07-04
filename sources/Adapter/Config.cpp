#include "Config.hpp"

namespace Technology_Adapter::Modbus::Config {

Readable::Readable(std::string name_, std::string description_,
    Information_Model::DataType type_, std::vector<int> registers_,
    Decoder decode_)
    : name(std::move(name_)), description(std::move(description_)), type(type_),
      registers(std::move(registers_)), decode(std::move(decode_)) {}

Group::Group(std::string name_, std::string description_)
    : name(std::move(name_)), description(std::move(description_)) {}

Device::Device(std::string id_, std::string name, std::string description,
    int slave_id_, size_t burst_size_,
    std::vector<RegisterRange> holding_registers_,
    std::vector<RegisterRange> input_registers_)
    : Group(std::move(name), std::move(description)), id(std::move(id_)),
      slave_id(slave_id_), burst_size(burst_size_),
      holding_registers(std::move(holding_registers_)),
      input_registers(std::move(input_registers_)) {}

Bus::Bus(std::string serial_port_, int baud_, LibModbus::Parity parity_,
    int data_bits_, int stop_bits_)
    : serial_port(std::move(serial_port_)), baud(baud_), parity(parity_),
      data_bits(data_bits_), stop_bits(stop_bits_) {}

} // namespace Technology_Adapter::Modbus::Config
