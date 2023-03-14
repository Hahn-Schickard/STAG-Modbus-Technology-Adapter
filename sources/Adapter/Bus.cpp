#include "Bus.hpp"

namespace Modbus_Technology_Adapter {

Bus::Bus(Config::Device const& config)
    : context_(config.serial_port, config.baud, config.parity, //
          config.data_bits, config.stop_bits),
      config_(config) {}

void Bus::buildModel(
    NonemptyPointer::NonemptyPtr<Technology_Adapter::DeviceBuilderPtr> const&
        device_builder,
    NonemptyPointer::NonemptyPtr<Technology_Adapter::ModelRegistryPtr> const&
        model_registry) {

  device_builder->buildDeviceBase(
      config_.id, config_.name, config_.description);
  buildGroup(device_builder, "",
      NonemptyPointer::NonemptyPtr<BusPtr>(shared_from_this()), config_);
  model_registry->registerDevice(device_builder->getResult());
}

void Bus::start() { context_.connect(); }

void Bus::stop() { context_.close(); }

void Bus::buildGroup(
    NonemptyPointer::NonemptyPtr<Technology_Adapter::DeviceBuilderPtr> const&
        device_builder,
    std::string const& group_id,
    NonemptyPointer::NonemptyPtr<BusPtr> const& shared_this,
    Config::Group const& group) {

  for (auto const& readable : group.readables) {
    size_t num_registers = readable.registers.size();
    auto registers = std::make_shared<std::vector<uint16_t>>(num_registers);
    device_builder->addDeviceElement( //
        group_id, readable.name, readable.description,
        Information_Model::ElementType::READABLE, readable.type,
        [shared_this, readable /*kept alive by `shared_this`*/, num_registers,
            registers]() {
          shared_this->context_.setSlave(shared_this->config_.slave_id);
          for (size_t i = 0; i < num_registers; ++i) {
            int read = shared_this->context_.readRegisters(
                readable.registers[i], 1, &(*registers)[i]);
            if (read == 0)
              throw "Read failed";
          }
          return readable.decode(*registers);
        },
        std::nullopt, std::nullopt);
  }

  for (auto const& subgroup : group.subgroups) {
    std::string group_id = device_builder->addDeviceElementGroup(
        subgroup.name, subgroup.description);
    buildGroup(device_builder, group_id, shared_this, subgroup);
  }
}

} // namespace Modbus_Technology_Adapter
