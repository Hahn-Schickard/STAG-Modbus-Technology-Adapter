#include "internal/Bus.hpp"

#include "Burst.hpp"

namespace Technology_Adapter::Modbus {

constexpr size_t NUM_READ_ATTEMPTS = 3; // 0 would mean instant failure

Bus::Bus(Config::Bus const& config, Config::Portname const& actual_port)
    : config_(config), //
      logger_(HaSLI::LoggerManager::registerLogger(
          "Modbus Bus " + config.id + "@" + actual_port)),
      context_(actual_port, config.baud, config.parity, config.data_bits,
          config.stop_bits) {}

void Bus::buildModel(
    Information_Model::NonemptyDeviceBuilderInterfacePtr const& device_builder,
    Technology_Adapter::NonemptyDeviceRegistryPtr const& model_registry) {

  logger_->info("Registering all devices");
  std::vector<std::string> added;

  try {
    for (auto const& device : config_.devices) {
      device_builder->buildDeviceBase(
          device.id, device.name, device.description);
      RegisterSet holding_registers(device.holding_registers);
      RegisterSet input_registers(device.input_registers);
      buildGroup(device_builder, model_registry,  "", //
          NonemptyPtr(shared_from_this()), //
          device, holding_registers, input_registers, device);
      if (model_registry->registrate(Information_Model::NonemptyDevicePtr(
              device_builder->getResult()))) {

        try {
          added.push_back(device.id);
        } catch (...) {
          // `push_back` failed. This must be an out-of-memory.
          model_registry->deregistrate(device.id);
          throw std::bad_alloc();
        }
      };
    }
  } catch (...) {
    // Before re-throwing, deregister everything that has been registered.
    for (auto const& device : added) {
      model_registry->deregistrate(device);
    }
    throw;
  }
}

void Bus::start() { context_.lock()->connect(); }

void Bus::stop() { context_.lock()->close(); }

void Bus::buildGroup(
    Information_Model::NonemptyDeviceBuilderInterfacePtr const& device_builder,
    Technology_Adapter::NonemptyDeviceRegistryPtr const& model_registry,
    std::string const& group_id, //
    NonemptyPtr const& shared_this, //
    Config::Device const& device, //
    RegisterSet const& holding_registers, RegisterSet const& input_registers,
    Config::Group const& group) {

  int slave_id = device.slave_id;
  auto& device_id = device.id;

  for (auto const& readable : group.readables) {
    auto buffer = std::make_shared<BurstBuffer>( //
        readable.registers, //
        holding_registers, input_registers, //
        device.burst_size);

    auto id = std::make_shared<std::string>();

    *id = device_builder->addReadableMetric( //
        group_id, readable.name, readable.description, readable.type,
        [shared_this, slave_id, &readable /*kept alive by `shared_this`*/,
            buffer, id, model_registry, device_id]() {
          // begin body
          {
            auto accessor = shared_this->context_.lock();
            shared_this->logger_->debug("Reading {}", *id);
            accessor->setSlave(slave_id);

            uint16_t* read_dest = buffer->padded.data();
            for (auto const& burst : buffer->plan.bursts) {
              RegisterIndex first_register = burst.start_register;
              int num_remaining_registers = burst.num_registers;
              while (num_remaining_registers > 0) {
                int num_read = 0;
                size_t remaining_attempts = NUM_READ_ATTEMPTS;
                while ((num_read == 0) && (remaining_attempts > 0)) {
                  try {
                    num_read = accessor->readRegisters(first_register,
                        burst.type, num_remaining_registers, read_dest);
                    if (num_read == 0) {
                      shared_this->logger_->debug("Reading " + *id + " failed");
                      if (remaining_attempts > 1) {
                        shared_this->logger_->debug("Retrying to read " + *id);
                        // wait for next iteration
                      } else {
                        model_registry->deregistrate(device_id);
                        throw std::runtime_error("Reading " + *id + " failed");
                      }
                    }
                  } catch(LibModbus::ModbusError const& error) {
                    shared_this->logger_->debug(
                        "Reading " + *id + " failed: " + error.what());
                    if (error.retryFeasible() && remaining_attempts > 1) {
                      shared_this->logger_->debug("Retrying to read " + *id);
                      // wait for next iteration
                    } else {
                      model_registry->deregistrate(device_id);
                      throw;
                    }
                  }
                  --remaining_attempts;
                }
                // Now `num_read > 0`, because otherwise we have thrown
                first_register += num_read;
                num_remaining_registers -= num_read;
                read_dest += num_read;
              }
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
    buildGroup(device_builder, model_registry, group_id, shared_this, device, //
        holding_registers, input_registers, subgroup);
  }
}

} // namespace Technology_Adapter::Modbus
