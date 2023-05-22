#include "Bus.hpp"

#include "Burst.hpp"

namespace Technology_Adapter::Modbus {

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
    RegisterSet readable_registers(device.readable_registers);
    buildGroup(device_builder, "",
        NonemptyPointer::NonemptyPtr<BusPtr>(shared_from_this()), //
        device, readable_registers, device);
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
    Config::Device const& device, RegisterSet const& readable_registers,
    Config::Group const& group) {

  int slave_id = device.slave_id;

  for (auto const& readable : group.readables) {
    auto buffer = std::make_shared<BurstBuffer>(
        readable.registers, readable_registers, device.burst_size);

    device_builder->addDeviceElement( //
        group_id, readable.name, readable.description,
        Information_Model::ElementType::READABLE, readable.type,
        [shared_this, slave_id, readable /*kept alive by `shared_this`*/,
            buffer]() {
          // begin body
          {
            auto accessor = shared_this->context_.lock();
            accessor->setSlave(slave_id);

            uint16_t* read_dest = buffer->padded.data();
            for (auto const& burst : buffer->plan.bursts) {
              int num_read = accessor->readRegisters(
                  burst.start_register, burst.num_registers, read_dest);
              if (num_read < burst.num_registers)
                throw "Read failed";
              read_dest += burst.num_registers;
            }
          } // no need to hold the lock during decoding
          size_t compact_size = buffer->compact.size();
          for (size_t i = 0; i < compact_size; ++i)
            buffer->compact[i] = buffer->padded[buffer->plan.task_to_plan[i]];
          return readable.decode(buffer->compact);
        },
        std::nullopt, std::nullopt);
  }

  for (auto const& subgroup : group.subgroups) {
    std::string group_id = device_builder->addDeviceElementGroup(
        subgroup.name, subgroup.description);
    buildGroup(device_builder, group_id, shared_this, device,
        readable_registers, subgroup);
  }
}

} // namespace Technology_Adapter::Modbus
