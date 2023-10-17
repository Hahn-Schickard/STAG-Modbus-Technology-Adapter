#include <iostream>

#include "Config.hpp"

namespace Technology_Adapter::Modbus::Config {

// NOLINTBEGIN(readability-identifier-naming)

Readable::Readable(std::string name_, std::string description_,
    Information_Model::DataType type_, std::vector<int> registers_,
    Decoder decode_)
    : name(std::move(name_)), description(std::move(description_)), type(type_),
      registers(std::move(registers_)), decode(std::move(decode_)) {}

Group::Group(std::string name_, std::string description_,
    std::vector<Readable> readables_, std::vector<Group> subgroups_)
    : name(std::move(name_)), description(std::move(description_)),
      readables(std::move(readables_)), subgroups(std::move(subgroups_)) {}

Device::Device(std::string id_, std::string name, std::string description,
    std::vector<Readable> readables_, std::vector<Group> subgroups_,
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    int slave_id_, size_t burst_size_, size_t max_retries_, size_t retry_delay_,
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    std::vector<RegisterRange> const& holding_registers_,
    std::vector<RegisterRange> const& input_registers_)
    : Group(std::move(name), std::move(description), std::move(readables_),
          std::move(subgroups_)),
      id(std::move(id_)), slave_id(slave_id_), burst_size(burst_size_),
      max_retries(max_retries_), retry_delay(retry_delay_),
      holding_registers(holding_registers_), input_registers(input_registers_) {
}

std::string busId(std::vector<Device> const& devices) {
  if (devices.empty()) {
    return "-";
  } else {
    size_t length = devices.size() - 1;
    for (auto const& device : devices) {
      length += device.id.length();
    }

    auto i = devices.begin();
    std::string result = i->id;
    result.reserve(length);
    ++i;
    while (i != devices.end()) {
      result += '/';
      result += i->id;
      ++i;
    }
    return result;
  }
}

Bus::Bus(std::vector<std::string> possible_serial_ports_, int baud_,
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    LibModbus::Parity parity_, int data_bits_, int stop_bits_,
    std::vector<Device> devices_)
    : possible_serial_ports(std::move(possible_serial_ports_)), baud(baud_),
      parity(parity_), data_bits(data_bits_), stop_bits(stop_bits_),
      devices(std::move(devices_)), id(busId(devices)) {}

// NOLINTEND(readability-identifier-naming)

} // namespace Technology_Adapter::Modbus::Config
