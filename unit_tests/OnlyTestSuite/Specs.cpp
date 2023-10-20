#include "Specs.hpp"

namespace SpecsForTests {

using namespace Technology_Adapter::Modbus;

Config::Device::NonemptyPtr specToConfig(DeviceSpec&& device) {
  return Config::Device::NonemptyPtr::make( //
      device.id, device.id /* as `name` */, device.id /* as `description` */,
      std::vector<Config::Readable>(), std::vector<Config::Group>(),
      device.slave_id, //
      1 /* as `burst_size` */, //
      0 /* as max_retries */, //
      0 /* as retry_delay */, //
      std::move(device.holding_registers),
      std::move(device.input_registers));
}

Config::Bus::NonemptyPtr specToConfig(BusSpec&& bus) {
  std::vector<Config::Device::NonemptyPtr> devices;
  for (auto& device : bus.devices) {
    devices.push_back(specToConfig(std::move(device)));
  }
  return Config::Bus::NonemptyPtr::make( //
      bus.possible_ports, 9600, LibModbus::Parity::None, 8, 2,
      devices);
}

} // namespace SpecsForTests
