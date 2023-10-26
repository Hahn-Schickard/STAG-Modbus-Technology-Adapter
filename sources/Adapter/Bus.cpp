#include "internal/Bus.hpp"

#include <thread>

#include "Burst.hpp"

namespace Technology_Adapter::Modbus {

Bus::Bus(ModbusTechnologyAdapterInterface& owner,
    Config::Bus::NonemptyPtr const& config,
    LibModbus::Context::Factory context_factory,
    Config::Portname const& actual_port,
    // NOLINTNEXTLINE(modernize-pass-by-value)
    Technology_Adapter::NonemptyDeviceRegistryPtr const& model_registry)
    : owner_(owner), config_(config), actual_port_(actual_port), //
      logger_(HaSLI::LoggerManager::registerLogger(std::string(
          (std::string_view)("Modbus Bus " + config->id + "@" + actual_port)))),
      model_registry_(model_registry),
      connection_(context_factory(actual_port, *config)) {}

Bus::~Bus() {
  try {
    stop();
  } catch (...) {
  }
}

void Bus::buildModel(Information_Model::NonemptyDeviceBuilderInterfacePtr const&
        device_builder) {

  if (!connection_.lock()->connected) {
    throw std::runtime_error(
        ("Bus " + actual_port_ + " not connected").c_str());
  }

  logger_->info("Registering all devices on bus {}", actual_port_);

  try {
    for (auto const& device : config_->devices) {
      device_builder->buildDeviceBase( //
          std::string((std::string_view)device->id),
          std::string((std::string_view)device->name),
          std::string((std::string_view)device->description));
      RegisterSet holding_registers(device->holding_registers);
      RegisterSet input_registers(device->input_registers);
      buildGroup(device_builder, "", //
          NonemptyPtr(shared_from_this()), //
          device, holding_registers, input_registers, *device);

      {
        auto accessor = connection_.lock();
        try {
          if (accessor->connected) {
            accessor->devices_to_deregister.push_back(device->id);
          }
        } catch (...) {
          // `push_back` failed. This must be an out-of-memory.
          throw std::bad_alloc();
        }
      }

      model_registry_->registrate(
          Information_Model::NonemptyDevicePtr(device_builder->getResult()));
    }
  } catch (std::exception const& exception) {
    auto accessor = connection_.lock();
    abort(accessor,
        "Deregistered all Modbus devices on bus " + actual_port_ +
            " after: " + exception.what());
  } catch (...) {
    auto accessor = connection_.lock();
    abort(accessor,
        "Deregistered all Modbus devices on bus " + actual_port_ +
            " after a non-standard exception");
  }
}

void Bus::start() {
  try {
    auto accessor = connection_.lock();
    accessor->context->connect();
    accessor->connected = true;
  } catch (std::exception const& exception) {
    throw std::runtime_error(
        ("Starting bus " + actual_port_ + "failed: " + exception.what())
            .c_str());
  } catch (...) {
    throw std::runtime_error( //
        ("Starting bus " + actual_port_ +
            "failed after a non-standard exception")
            .c_str());
  }
}

void Bus::stop() {
  auto accessor = connection_.lock();
  stop(accessor);
}

// This has become too long to be a lambda. Hence a `Callable`
struct Readcallback {
private:
  Bus::NonemptyPtr const bus_;
  Config::Device::NonemptyPtr const device_;

  // initialized after the constructor but before `DeviceRegistry::registrate`
  std::shared_ptr<std::string> const metric_id_;

  Config::Readable const readable_;
  NonemptyPointer::NonemptyPtr<std::shared_ptr<BurstBuffer>> const buffer_;

public:
  Readcallback( //
      Bus::NonemptyPtr const& bus, // NOLINT(modernize-pass-by-value)
      // NOLINTNEXTLINE(modernize-pass-by-value)
      Config::Device::NonemptyPtr const& device,
      std::shared_ptr<std::string> metric_id, //
      Config::Readable readable,
      // NOLINTNEXTLINE(modernize-pass-by-value)
      NonemptyPointer::NonemptyPtr<std::shared_ptr<BurstBuffer>> const& buffer)
      : bus_(bus), device_(device), metric_id_(std::move(metric_id)),
        readable_(std::move(readable)), buffer_(buffer) {}

  Information_Model::DataVariant operator()() const {
    {
      auto accessor = bus_->connection_.lock();
      if (accessor->connected) {
        bus_->logger_->debug("Reading {}", *metric_id_);
        accessor->context->selectDevice(*device_);

        uint16_t* read_dest = buffer_->padded.data();
        for (auto const& burst : buffer_->plan.bursts) {
          readBurst(accessor, burst, read_dest);
          read_dest += burst.num_registers;
        }
      } else {
        // Some other thread closed the connection. Hence the resource has been
        // deregistered.
        bus_->logger_->debug(
            "Reading {} failed because the connection was closed", *metric_id_);
        throw std::runtime_error(
            (device_->id + " has been deregistered").c_str());
      }
    } // no need to hold the lock during decoding
    size_t compact_size = buffer_->compact.size();
    for (size_t i = 0; i < compact_size; ++i) {
      buffer_->compact[i] = buffer_->padded[buffer_->plan.task_to_plan[i]];
    }
    return readable_.decode(buffer_->compact);
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
    size_t remaining_attempts = device_->max_retries + 1;
    while ((num_read == 0) && (remaining_attempts > 0)) {
      try {
        num_read = accessor->context->readRegisters(
            first_register, burst.type, num, read_dest);
        if (num_read == 0) {
          bus_->logger_->debug("Reading {} failed", *metric_id_);
          retryOrAbort(remaining_attempts, accessor,
              "Deregistered " + device_->id +
                  " after too many read attempts for " + *metric_id_);
        }
      } catch (LibModbus::ModbusError const& error) {
        bus_->logger_->debug(
            "Reading {} failed: {}", *metric_id_, error.what());
        if (error.retryFeasible()) {
          retryOrAbort(remaining_attempts, accessor,
              "Deregistered " + device_->id +
                  " after too many read attempts for " + *metric_id_ +
                  ". Last error was: " + error.what());
        } else {
          bus_->abort(accessor,
              "Deregistered " + device_->id + " after: " + error.what());
        }
      }
      --remaining_attempts;
    }
    // Now `num_read > 0`, because otherwise we have thrown
    return num_read;
  }

  // retrying will happen after the function returns without throwing
  void retryOrAbort( //
      size_t& remaining_attempts,
      Bus::ConnectionResource::ScopedAccessor& accessor,
      ConstString::ConstString const& error_message) const {

    if (remaining_attempts > 1) {
      if (device_->retry_delay > 0) {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(device_->retry_delay));
      }
      bus_->logger_->debug("Retrying to read {}", *metric_id_);
      // wait for next iteration of `ReadRegisters`
    } else {
      bus_->abort(accessor, error_message);
    }
  }
};

void Bus::buildGroup(
    Information_Model::NonemptyDeviceBuilderInterfacePtr const& device_builder,
    std::string const& group_id, //
    NonemptyPtr const& shared_this, //
    Config::Device::NonemptyPtr const& device, //
    RegisterSet const& holding_registers, RegisterSet const& input_registers,
    Config::Group const& group) {

  for (auto const& readable : group.readables) {
    auto buffer = NonemptyPointer::make_shared<BurstBuffer>( //
        readable.registers, //
        holding_registers, input_registers, //
        device->burst_size);

    auto metric_id = std::make_shared<std::string>();

    *metric_id = device_builder->addReadableMetric( //
        group_id, std::string((std::string_view)readable.name),
        std::string((std::string_view)readable.description), readable.type,
        Readcallback(shared_this, device, metric_id, readable, buffer));
  }

  for (auto const& subgroup : group.subgroups) {
    std::string group_id = device_builder->addDeviceElementGroup(
        std::string((std::string_view)subgroup.name),
        std::string((std::string_view)subgroup.description));
    buildGroup(device_builder, group_id, shared_this, device, //
        holding_registers, input_registers, subgroup);
  }
}

void Bus::stop(ConnectionResource::ScopedAccessor& accessor) {
  logger_->trace("Stopping bus {}", actual_port_);
  for (auto const& device : accessor->devices_to_deregister) {
    model_registry_->deregistrate(std::string((std::string_view)device));
  }
  accessor->devices_to_deregister.clear();
  if (accessor->connected) {
    accessor->context->close();
    accessor->connected = false;
  }
}

[[noreturn]] void Bus::abort(ConnectionResource::ScopedAccessor& accessor,
    ConstString::ConstString const& error_message) {

  logger_->trace("Aborting bus {}", actual_port_);
  stop(accessor);
  owner_.cancelBus(actual_port_);
  throw std::runtime_error(error_message.c_str());
}

} // namespace Technology_Adapter::Modbus
