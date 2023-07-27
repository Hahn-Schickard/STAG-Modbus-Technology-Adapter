#include "Bus.hpp"

#include "Burst.hpp"

namespace Technology_Adapter::Modbus {

Bus::Bus(Config::Bus const& config)
    : context_(config.possible_serial_ports.at(0), config.baud, config.parity,
          config.data_bits, config.stop_bits),
      config_(config) {}

void Bus::buildModel(
    Information_Model::NonemptyDeviceBuilderInterfacePtr const& device_builder,
    Technology_Adapter::NonemptyDeviceRegistryPtr const& model_registry) {

  for (auto const& device : config_.devices) {
    device_builder->buildDeviceBase(device.id, device.name, device.description);
    RegisterSet holding_registers(device.holding_registers);
    RegisterSet input_registers(device.input_registers);
    buildGroup(device_builder, "", //
        NonemptyPtr(shared_from_this()), //
        device, holding_registers, input_registers, device);
    model_registry->registrate(
        Information_Model::NonemptyDevicePtr(device_builder->getResult()));
  }
}

void Bus::start() { context_.lock()->connect(); }

void Bus::stop() { context_.lock()->close(); }

void Bus::buildGroup(
    Information_Model::NonemptyDeviceBuilderInterfacePtr const& device_builder,
    std::string const& group_id, //
    NonemptyPtr const& shared_this, //
    Config::Device const& device, //
    RegisterSet const& holding_registers, RegisterSet const& input_registers,
    Config::Group const& group) {

  int slave_id = device.slave_id;

  for (auto const& readable : group.readables) {
    auto buffer = std::make_shared<BurstBuffer>( //
        readable.registers, //
        holding_registers, input_registers, //
        device.burst_size);

    device_builder->addReadableMetric( //
        group_id, readable.name, readable.description, readable.type,
        [shared_this, slave_id, readable /*kept alive by `shared_this`*/,
            buffer]() {
          // begin body
          {
            auto accessor = shared_this->context_.lock();
            accessor->setSlave(slave_id);

            uint16_t* read_dest = buffer->padded.data();
            for (auto const& burst : buffer->plan.bursts) {
              int num_read = accessor->readRegisters(burst.start_register,
                  burst.type, burst.num_registers, read_dest);
              if (num_read < burst.num_registers) {
                throw "Read failed";
              }
              read_dest += burst.num_registers;
            }
          } // no need to hold the lock during decoding
          size_t compact_size = buffer->compact.size();
          for (size_t i = 0; i < compact_size; ++i) {
            buffer->compact[i] = buffer->padded[buffer->plan.task_to_plan[i]];
          }
          return readable.decode(buffer->compact);
        });
  }

  for (auto const& subgroup : group.subgroups) {
    std::string group_id = device_builder->addDeviceElementGroup(
        subgroup.name, subgroup.description);
    buildGroup(device_builder, group_id, shared_this, device, //
        holding_registers, input_registers, subgroup);
  }
}

} // namespace Technology_Adapter::Modbus
