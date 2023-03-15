#include "Bus.hpp"

namespace Modbus_Technology_Adapter {

Bus::Bus(Config::Bus const& config)
    : context_(config.serial_port, config.baud, config.parity, //
          config.data_bits, config.stop_bits),
      config_(config) {}

void Bus::buildModel(
    NonemptyPointer::NonemptyPtr<Technology_Adapter::DeviceBuilderPtr> const&
        device_builder,
    NonemptyPointer::NonemptyPtr<Technology_Adapter::ModelRegistryPtr> const&
        model_registry) {

  for (auto const& device : config_.devices) {
    device_builder->buildDeviceBase(device.id, device.name, device.description);
    buildGroup(device_builder, "",
        NonemptyPointer::NonemptyPtr<BusPtr>(shared_from_this()), //
        device, device);
    model_registry->registerDevice(device_builder->getResult());
  }
}

void Bus::start() { context_.lock()->connect(); }

void Bus::stop() { context_.lock()->close(); }

void Bus::buildGroup(
    NonemptyPointer::NonemptyPtr<Technology_Adapter::DeviceBuilderPtr> const&
        device_builder,
    std::string const& group_id,
    NonemptyPointer::NonemptyPtr<BusPtr> const& shared_this,
    Config::Device const& device, Config::Group const& group) {

  int slave_id = device.slave_id;

  for (auto const& readable : group.readables) {
    size_t num_registers = readable.registers.size();
    auto registers = std::make_shared<std::vector<uint16_t>>(num_registers);
    device_builder->addDeviceElement( //
        group_id, readable.name, readable.description,
        Information_Model::ElementType::READABLE, readable.type,
        [shared_this, slave_id, readable /*kept alive by `shared_this`*/,
            num_registers, registers]() {
          // begin body
          {
            auto accessor = shared_this->context_.lock();
            accessor->setSlave(slave_id);
            for (size_t i = 0; i < num_registers; ++i) {
              int read = accessor->readRegisters(
                  readable.registers[i], 1, &(*registers)[i]);
              if (read == 0)
                throw "Read failed";
            }
          } // no need to hold the lock during decoding
          return readable.decode(*registers);
        },
        std::nullopt, std::nullopt);
  }

  for (auto const& subgroup : group.subgroups) {
    std::string group_id = device_builder->addDeviceElementGroup(
        subgroup.name, subgroup.description);
    buildGroup(device_builder, group_id, shared_this, device, subgroup);
  }
}

} // namespace Modbus_Technology_Adapter
