#include "internal/Bus.hpp"
#include "Burst.hpp"
#include "ModbusTechnologyAdapter.hpp"

namespace Technology_Adapter::Modbus {

constexpr size_t NUM_READ_ATTEMPTS = 3; // 0 would mean instant failure

Bus::Bus(ModbusTechnologyAdapter& owner, Config::Bus const& config,
    Config::Portname const& actual_port,
    // NOLINTNEXTLINE(modernize-pass-by-value)
    Technology_Adapter::NonemptyDeviceRegistryPtr const& model_registry)
    : owner_(owner), config_(config), actual_port_(actual_port), //
      logger_(HaSLI::LoggerManager::registerLogger(
          "Modbus Bus " + config.id + "@" + actual_port)),
      model_registry_(model_registry),
      connection_(actual_port, config.baud, config.parity, config.data_bits,
          config.stop_bits) {}

Bus::~Bus() {
  try {
    stop();
  } catch (...) {
  }
}

void Bus::buildModel(Information_Model::NonemptyDeviceBuilderInterfacePtr const&
        device_builder) {

  logger_->info("Registering all devices on bus {}", actual_port_);

  auto accessor = connection_.lock();

  try {
    for (auto const& device : config_.devices) {
      device_builder->buildDeviceBase(
          device.id, device.name, device.description);
      RegisterSet holding_registers(device.holding_registers);
      RegisterSet input_registers(device.input_registers);
      buildGroup(device_builder, "", //
          NonemptyPtr(shared_from_this()), //
          device, holding_registers, input_registers, device);
      if (model_registry_->registrate(Information_Model::NonemptyDevicePtr(
              device_builder->getResult()))) {

        try {
          accessor->registered_devices.push_back(device.id);
        } catch (...) {
          // `push_back` failed. This must be an out-of-memory.
          model_registry_->deregistrate(device.id);
          throw std::bad_alloc();
        }
      };
    }
  } catch (std::exception const& exception) {
    abort(accessor,
        "Deregistered all Modbus devices on bus " + actual_port_ +
            " after: " + exception.what());
  } catch (...) {
    abort(accessor,
        "Deregistered all Modbus devices on bus " + actual_port_ +
            " after a non-standard exception");
  }
}

void Bus::start() {
  try {
    auto accessor = connection_.lock();
    accessor->context.connect();
    accessor->connected = true;
  } catch (...) {
    stop();
    throw;
  }
}

void Bus::stop() {
  auto accessor = connection_.lock();
  stop(accessor);
}

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
      auto accessor = bus->connection_.lock();
      if (accessor->connected) {
        bus->logger_->debug("Reading {}", *metric_id);
        accessor->context.setSlave(slave_id);

        uint16_t* read_dest = buffer->padded.data();
        for (auto const& burst : buffer->plan.bursts) {
          readBurst(accessor, burst, read_dest);
          read_dest += burst.num_registers;
        }
      } else {
        // Some other thread closed the connection. Hence the resource has been
        // deregistered.
        bus->logger_->debug(
            "Reading {} failed because the connection was closed", *metric_id);
        throw std::runtime_error(device_id + " has been deregistered");
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
      Bus::ConnectionResource::ScopedAccessor& accessor,
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
      Bus::ConnectionResource::ScopedAccessor& accessor,
      BurstPlan::Burst const& burst, //
      uint16_t* const read_dest, //
      RegisterIndex first_register, //
      int num) const {
    int num_read = 0;
    size_t remaining_attempts = NUM_READ_ATTEMPTS;
    while ((num_read == 0) && (remaining_attempts > 0)) {
      try {
        num_read = accessor->context.readRegisters(
            first_register, burst.type, num, read_dest);
        if (num_read == 0) {
          bus->logger_->debug("Reading {} failed", *metric_id);
          if (remaining_attempts > 1) {
            bus->logger_->debug("Retrying to read {}", *metric_id);
            // wait for next iteration
          } else {
            bus->abort(accessor,
                "Deregistered " + device_id + " after too many read attempts");
          }
        }
      } catch (LibModbus::ModbusError const& error) {
        bus->logger_->debug("Reading {} failed: {}", *metric_id, error.what());
        if (error.retryFeasible()) {
          if (remaining_attempts > 1) {
            bus->logger_->debug("Retrying to read {}", *metric_id);
            // wait for next iteration
          } else {
            bus->abort(accessor,
                "Deregistered " + device_id +
                    " after too many read attempts. Last error was: " +
                    error.what());
          }
        } else {
          bus->abort(accessor,
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
        group_id, readable.name, readable.description, readable.type,
        Readcallback{model_registry_, shared_this, slave_id, device_id,
            metric_id, readable, buffer});
  }

  for (auto const& subgroup : group.subgroups) {
    std::string group_id = device_builder->addDeviceElementGroup(
        subgroup.name, subgroup.description);
    buildGroup(device_builder, group_id, shared_this, device, //
        holding_registers, input_registers, subgroup);
  }
}

void Bus::stop(ConnectionResource::ScopedAccessor& accessor) {
  logger_->trace("Stopping bus {}", actual_port_);
  for (auto const& device : accessor->registered_devices) {
    model_registry_->deregistrate(device);
  }
  accessor->registered_devices.clear();
  if (accessor->connected) {
    accessor->context.close();
    accessor->connected = false;
  }
}

void Bus::abort(ConnectionResource::ScopedAccessor& accessor,
    std::string const& error_message) {

  logger_->trace("Aborting bus {}", actual_port_);
  stop(accessor);
  owner_.cancelBus(actual_port_);
  throw std::runtime_error(error_message);
}

} // namespace Technology_Adapter::Modbus
