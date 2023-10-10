#include "internal/Bus.hpp"
#include "Burst.hpp"

namespace Technology_Adapter::Modbus {

constexpr size_t NUM_READ_ATTEMPTS = 3; // 0 would mean instant failure

Bus::Bus(Config::Bus const& config, Config::Portname const& actual_port)
    : config_(config), actual_port_(actual_port), //
      logger_(HaSLI::LoggerManager::registerLogger(std::string(
          ("Modbus Bus " + config.id + "@" + actual_port).c_str()))),
      context_(actual_port, config.baud, config.parity, config.data_bits,
          config.stop_bits) {}

void Bus::buildModel(
    Information_Model::NonemptyDeviceBuilderInterfacePtr const& device_builder,
    Technology_Adapter::NonemptyDeviceRegistryPtr const& model_registry) {

  logger_->info("Registering all devices on bus {}", actual_port_);
  std::vector<std::string> added;

  try {
    try {
      for (auto const& device : config_.devices) {
        device_builder->buildDeviceBase(
            std::string(device.id.c_str()), std::string(device.name.c_str()),
            std::string(device.description.c_str()));
        RegisterSet holding_registers(device.holding_registers);
        RegisterSet input_registers(device.input_registers);
        buildGroup(device_builder, model_registry, "", //
            NonemptyPtr(shared_from_this()), //
            device, holding_registers, input_registers, device);
        if (model_registry->registrate(Information_Model::NonemptyDevicePtr(
                device_builder->getResult()))) {

          try {
            added.push_back(std::string(device.id.c_str()));
          } catch (...) {
            // `push_back` failed. This must be an out-of-memory.
            model_registry->deregistrate(std::string(device.id.c_str()));
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
  } catch (std::exception const& exception) {
    throw std::runtime_error( //
        ("Deregistered all Modbus devices on bus " + actual_port_ +
            " after: " + exception.what()).c_str());
  } catch (...) {
    throw std::runtime_error( //
        ("Deregistered all Modbus devices on bus " + actual_port_ +
            " after a non-standard exception").c_str());
  }
}

void Bus::start() { context_.lock()->connect(); }

void Bus::stop() { context_.lock()->close(); }

// This has become too long to be a lambda. Hence a `Callable`
struct Readcallback {
  Technology_Adapter::NonemptyDeviceRegistryPtr const model_registry;
  Bus::NonemptyPtr const bus;
  int const slave_id;
  std::string const device_id;
  std::shared_ptr<std::string> const metric_id; // to be initialized later
  Config::Readable const readable;
  NonemptyPointer::NonemptyPtr<std::shared_ptr<BurstBuffer>> const buffer;

  Information_Model::DataVariant operator()() const {
    {
      auto accessor = bus->context_.lock();
      bus->logger_->debug("Reading {}", *metric_id);
      accessor->setSlave(slave_id);

      uint16_t* read_dest = buffer->padded.data();
      for (auto const& burst : buffer->plan.bursts) {
        readBurst(accessor, burst, read_dest);
        read_dest += burst.num_registers;
      }
    } // no need to hold the lock during decoding
    size_t compact_size = buffer->compact.size();
    for (size_t i = 0; i < compact_size; ++i) {
      buffer->compact[i] = buffer->padded[buffer->plan.task_to_plan[i]];
    }
    return readable.decode(buffer->compact);
  }

private:
  void readBurst( //
      Threadsafe::Resource<LibModbus::ContextRTU,
          Threadsafe::QueuedMutex>::ScopedAccessor& accessor,
      BurstPlan::Burst const& burst, //
      uint16_t* read_dest) const {

    RegisterIndex first_register = burst.start_register;
    int num_remaining_registers = burst.num_registers;
    while (num_remaining_registers > 0) {
      int num_read = readRegisters(
          accessor, burst, read_dest, first_register, num_remaining_registers);
      first_register += num_read;
      num_remaining_registers -= num_read;
      read_dest += num_read;
    }
  }

  // returns the number of registers actually read. That number is > 0
  int readRegisters( //
      Threadsafe::Resource<LibModbus::ContextRTU,
          Threadsafe::QueuedMutex>::ScopedAccessor& accessor,
      BurstPlan::Burst const& burst, //
      uint16_t* const read_dest, //
      RegisterIndex first_register, //
      int num) const {
    int num_read = 0;
    size_t remaining_attempts = NUM_READ_ATTEMPTS;
    while ((num_read == 0) && (remaining_attempts > 0)) {
      try {
        num_read =
            accessor->readRegisters(first_register, burst.type, num, read_dest);
        if (num_read == 0) {
          bus->logger_->debug("Reading {} failed", *metric_id);
          if (remaining_attempts > 1) {
            bus->logger_->debug("Retrying to read {}", *metric_id);
            // wait for next iteration
          } else {
            model_registry->deregistrate(device_id);
            throw std::runtime_error("Deregistered " + device_id +
                " after reading " + *metric_id + " failed");
          }
        }
      } catch (LibModbus::ModbusError const& error) {
        bus->logger_->debug("Reading {} failed: {}", *metric_id, error.what());
        if (error.retryFeasible()) {
          if (remaining_attempts > 1) {
            bus->logger_->debug("Retrying to read {}", *metric_id);
            // wait for next iteration
          } else {
            model_registry->deregistrate(device_id);
            throw std::runtime_error( //
                "Deregistered " + device_id +
                " after too many read attempts. Last error was: " +
                error.what());
          }
        } else {
          model_registry->deregistrate(device_id);
          throw std::runtime_error(
              "Deregistered " + device_id + " after: " + error.what());
        }
      }
      --remaining_attempts;
    }
    // Now `num_read > 0`, because otherwise we have thrown
    return num_read;
  }
};

void Bus::buildGroup(
    Information_Model::NonemptyDeviceBuilderInterfacePtr const& device_builder,
    Technology_Adapter::NonemptyDeviceRegistryPtr const& model_registry,
    std::string const& group_id, //
    NonemptyPtr const& shared_this, //
    Config::Device const& device, //
    RegisterSet const& holding_registers, RegisterSet const& input_registers,
    Config::Group const& group) {

  int slave_id = device.slave_id;
  auto const& device_id = device.id;

  for (auto const& readable : group.readables) {
    auto buffer = NonemptyPointer::make_shared<BurstBuffer>( //
        readable.registers, //
        holding_registers, input_registers, //
        device.burst_size);

    auto metric_id = std::make_shared<std::string>();

    *metric_id = device_builder->addReadableMetric( //
        group_id, std::string(readable.name.c_str()),
        std::string(readable.description.c_str()), readable.type,
        Readcallback{model_registry, shared_this, slave_id,
            std::string(device_id.c_str()), metric_id, readable, buffer});
  }

  for (auto const& subgroup : group.subgroups) {
    std::string group_id = device_builder->addDeviceElementGroup(
        std::string(subgroup.name.c_str()),
        std::string(subgroup.description.c_str()));
    buildGroup(device_builder, model_registry, group_id, shared_this, device, //
        holding_registers, input_registers, subgroup);
  }
}

} // namespace Technology_Adapter::Modbus
