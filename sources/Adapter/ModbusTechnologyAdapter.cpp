#include "ModbusTechnologyAdapter.hpp"

namespace Modbus_Technology_Adapter {

ModbusTechnologyAdapter::ModbusTechnologyAdapter(Config::Device&& config)
    : Technology_Adapter::TechnologyAdapter("Modbus Adapter"),
      bus_(config.serial_port, config.baud, config.parity, //
          config.data_bits, config.stop_bits),
      config_(config) {}

void ModbusTechnologyAdapter::interfaceSet() {

  // model the device in `builder_interface_`

  Technology_Adapter::DeviceBuilderPtr device_builder = getDeviceBuilder();
  device_builder->buildDeviceBase(
      config_.id, config_.name, config_.description);

  for (auto const& readable : config_.readables) {
    size_t num_registers = readable.registers.size();
    auto registers = std::make_shared<std::vector<uint16_t>>(num_registers);
    device_builder->addReadableMetric(readable.name, readable.description,
        readable.type, //
        [this, readable, num_registers, registers]() {
          bus_.setSlave(config_.slave_id);
          for (size_t i = 0; i < num_registers; ++i) {
            int read =
                bus_.readRegisters(readable.registers[i], 1, &(*registers)[i]);
            if (read == 0)
              throw "Read failed";
          }
          return readable.decode(*registers);
        });
  }

  for (auto const& subgroup : config_.subgroups) {
    std::string group_id = device_builder->addDeviceElementGroup(
        subgroup.name, subgroup.description);
    registerSubgroupContents(device_builder, group_id, subgroup);
  }

  // register device model with `registry_interface_`
  getModelRegistry()->registerDevice(device_builder->getResult());
}

void ModbusTechnologyAdapter::registerSubgroupContents(
    Technology_Adapter::DeviceBuilderPtr const& device_builder,
    std::string const& id, Config::Group const& group) {

  for (auto const& readable : group.readables) {
    size_t num_registers = readable.registers.size();
    auto registers = std::make_shared<std::vector<uint16_t>>(num_registers);
    device_builder->addReadableMetric(id, readable.name, readable.description,
        readable.type, //
        [this, readable, num_registers, registers]() {
          bus_.setSlave(config_.slave_id);
          for (size_t i = 0; i < num_registers; ++i) {
            int read =
                bus_.readRegisters(readable.registers[i], 1, &(*registers)[i]);
            if (read == 0)
              throw "Read failed";
          }
          return readable.decode(*registers);
        });
  }

  for (auto const& subgroup : group.subgroups) {
    std::string group_id = device_builder->addDeviceElementGroup(
        id, subgroup.name, subgroup.description);
    registerSubgroupContents(device_builder, group_id, subgroup);
  }
}

void ModbusTechnologyAdapter::start() {
  bus_.connect();
  Technology_Adapter::TechnologyAdapter::start();
}

void ModbusTechnologyAdapter::stop() {
  Technology_Adapter::TechnologyAdapter::stop();
  bus_.close();
}

} // namespace Modbus_Technology_Adapter
