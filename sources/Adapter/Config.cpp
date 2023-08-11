#include "Config.hpp"

namespace Technology_Adapter::Modbus::Config {

// NOLINTBEGIN(readability-identifier-naming)

Readable::Readable(std::string name_, std::string description_,
    Information_Model::DataType type_, std::vector<int> registers_,
    Decoder decode_)
    : name(std::move(name_)), description(std::move(description_)), type(type_),
      registers(std::move(registers_)), decode(std::move(decode_)) {}

Group::Group(std::string name_, std::string description_)
    : name(std::move(name_)), description(std::move(description_)) {}

Device::Device(std::string id_, std::string name, std::string description,
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    int slave_id_, size_t burst_size_,
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    std::vector<RegisterRange> const& holding_registers_,
    std::vector<RegisterRange> const& input_registers_)
    : Group(std::move(name), std::move(description)), id(std::move(id_)),
      slave_id(slave_id_), burst_size(burst_size_),
      holding_registers(holding_registers_), input_registers(input_registers_) {
}

Bus::Bus(std::vector<std::string> possible_serial_ports_, int baud_,
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    LibModbus::Parity parity_, int data_bits_, int stop_bits_)
    : possible_serial_ports(std::move(possible_serial_ports_)), baud(baud_),
      parity(parity_), data_bits(data_bits_), stop_bits(stop_bits_) {}

// NOLINTEND(readability-identifier-naming)

} // namespace Technology_Adapter::Modbus::Config
