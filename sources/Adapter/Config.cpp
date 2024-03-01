#include "internal/Config.hpp"

namespace Technology_Adapter::Modbus::Config {

// NOLINTBEGIN(readability-identifier-naming)

Device::Device(ConstString::ConstString id_, ConstString::ConstString name,
    ConstString::ConstString description, std::vector<Readable> readables_,
    std::vector<Group> subgroups_,
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    int slave_id_, size_t burst_size_, size_t max_retries_, size_t retry_delay_,
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    std::vector<RegisterRange> const& holding_registers_,
    std::vector<RegisterRange> const& input_registers_)
    : Group{std::move(name), std::move(description), std::move(readables_),
          std::move(subgroups_)},
      id(std::move(id_)), slave_id(slave_id_), burst_size(burst_size_),
      max_retries(max_retries_), retry_delay(retry_delay_),
      holding_registers(holding_registers_), input_registers(input_registers_) {
}

/// @brief Creates a bus Id (for logging) from Ids of the bus' devices
ConstString::ConstString busId(
    std::vector<Device::NonemptyPtr> const& devices) {

  if (devices.empty()) {
    return ConstString::ConstString("-");
  } else {
    size_t length = devices.size() - 1;
    for (auto const& device : devices) {
      length += device->id.length();
    }

    auto i = devices.begin();
    std::string result((std::string_view)((*i)->id));
    result.reserve(length);
    ++i;
    while (i != devices.end()) {
      result += '/';
      result += (std::string_view)(*i)->id;
      ++i;
    }
    return ConstString::ConstString(result);
  }
}

Bus::Bus(std::vector<Portname> possible_serial_ports_, int baud_,
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    LibModbus::Parity parity_, int data_bits_, int stop_bits_, int rts_delay_,
    size_t inter_use_delay_when_searching_,
    size_t inter_use_delay_when_running_,
    size_t inter_device_delay_when_searching_,
    size_t inter_device_delay_when_running_,
    std::vector<Device::NonemptyPtr> devices_)
    : possible_serial_ports(std::move(possible_serial_ports_)), baud(baud_),
      parity(parity_), data_bits(data_bits_), stop_bits(stop_bits_),
      rts_delay(rts_delay_),
      inter_use_delay_when_searching(inter_use_delay_when_searching_),
      inter_use_delay_when_running(inter_use_delay_when_running_),
      inter_device_delay_when_searching(inter_device_delay_when_searching_),
      inter_device_delay_when_running(inter_device_delay_when_running_),
      devices(std::move(devices_)), id(busId(devices)) {}

// NOLINTEND(readability-identifier-naming)

} // namespace Technology_Adapter::Modbus::Config
