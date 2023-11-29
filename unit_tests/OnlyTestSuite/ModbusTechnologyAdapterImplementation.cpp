#include "gtest/gtest.h"

#include "Information_Model/mocks/DeviceMockBuilder.hpp"
#include "Technology_Adapter_Interface/mocks/ModelRepositoryInterface_MOCK.hpp"

#include "internal/ConfigJson.hpp"
#include "internal/ModbusTechnologyAdapterImplementation.hpp"
#include "internal/ModbusTechnologyAdapterInterface.hpp"

#include "VirtualContext.hpp"

namespace Information_Model {

bool operator==(DataVariant const& x, double y) {
  return ((toDataType(x) == DataType::DOUBLE) && (std::get<double>(x) == y));
}

} // namespace Information_Model

namespace ModbusTechnologyAdapterTests::
    ModbusTechnologyAdapterImplementationTests {

using namespace Technology_Adapter::Modbus;
using namespace Virtual_Context;

// NOLINTBEGIN(cert-err58-cpp, readability-magic-numbers)

auto long_time = std::chrono::milliseconds(100);

ConstString::ConstString device_id{"The device"};
ConstString::ConstString port_name{"The port"};
ConstString::ConstString other_port_name{"Other port"};

// clang-format off
auto buses_config = Config::BusesOfJson({{
  {"possible_serial_ports", {"The port", "Other port"}},
  {"devices", {
    {
      {"slave_id", 10},
      {"id", "The device"},
      {"name", "N"},
      {"description", "D"},
      {"holding_registers", {
        {{"begin", 2}, {"end", 3}},
      }},
      {"input_registers", nlohmann::json::array()},
      {"burst_size", 1},
      {"elements", {
        {
          {"element_type", "readable"},
          {"name", "N1"},
          {"description", "D1"},
          {"registers", {3,2}},
          {"decoder", {
            {"type", "linear"},
            {"factor", 2},
            {"offset", 1},
          }},
        },
      }},
    },
  }},
  {"baud", 1},
  {"parity", "None"},
  {"stop_bits", 2},
  {"data_bits", 3},
}});
// clang-format on

// A proxy to an actual `ModbusTechnologyAdapterImplementation`
// with method call counting and extra callbacks
struct Adapter : public ModbusTechnologyAdapterImplementation {
  using StartCallback = std::function<void()>;
  using StopCallback = std::function<void()>;
  using AddBusCallback = std::function<void(
      Config::Bus::NonemptyPtr const&, Config::Portname const&)>;
  using CancelBusCallback = std::function<void(Config::Portname const&)>;

  Adapter(VirtualContext::Factory context_factory)
      : ModbusTechnologyAdapterImplementation{
            std::move(context_factory), buses_config} {}

  StartCallback start_callback = []() {};
  StopCallback stop_callback = []() {};
  AddBusCallback add_bus_callback = //
      [](Config::Bus::NonemptyPtr const&, Config::Portname const&) {};
  CancelBusCallback cancel_bus_callback = [](Config::Portname const&) {};

  size_t start_called = 0;
  size_t stop_called = 0;
  size_t add_bus_called = 0;
  size_t cancel_bus_called = 0;

  void start() final {
    ++start_called;
    start_callback();
    ModbusTechnologyAdapterImplementation::start();
  }

  void stop() final {
    ++stop_called;
    stop_callback();
    ModbusTechnologyAdapterImplementation::stop();
  }

  void addBus(Config::Bus::NonemptyPtr const& bus,
      Config::Portname const& actual_port) final {

    ++add_bus_called;
    add_bus_callback(bus, actual_port);
    ModbusTechnologyAdapterImplementation::addBus(bus, actual_port);
  }

  void cancelBus(Config::Portname const& port) final {
    ++cancel_bus_called;
    cancel_bus_callback(port);
    ModbusTechnologyAdapterImplementation::cancelBus(port);
  }
};

struct ModbusTechnologyAdapterImplementationTests : public testing::Test {
  using ReadFunction = std::function<Information_Model::DataVariant()>;
  using RegistrationCallback = std::function<void(ReadFunction const& metric)>;
  using RegistrationCallback_ =
      std::function<void(Information_Model::NonemptyDevicePtr const& device)>;

  RegistrationCallback registration_callback = [](ReadFunction const&) {};

  size_t registration_called = 0;
  size_t deregistration_called = 0;

  Information_Model::NonemptyDeviceBuilderInterfacePtr device_builder{
      std::make_shared<Information_Model::testing::DeviceMockBuilder>()};

  NonemptyPointer::NonemptyPtr<
      Technology_Adapter::testing::ModelRepositoryMockPtr>
      model_repository{std::make_shared<::testing::NiceMock<
          Technology_Adapter::testing::ModelRepositoryMock>>(
          [this](Information_Model::NonemptyDevicePtr device) -> bool {
            ++registration_called;
            auto device_group = device->getDeviceElementGroup();
            auto elements = device_group->getSubelements();
            EXPECT_EQ(elements.size(), 1);
            auto element = elements.at(0);
            EXPECT_EQ(element->getElementType(),
                Information_Model::ElementType::READABLE);
            auto metric = std::get<Information_Model::NonemptyMetricPtr>(
                element->functionality);
            registration_callback(
                [metric]() { return metric->getMetricValue(); });
            return true;
          },
          [this](std::string const&) -> bool {
            ++deregistration_called;
            return true;
          })};

  Technology_Adapter::NonemptyDeviceRegistryPtr device_registry{
      std::make_shared<Technology_Adapter::DeviceRegistry>(model_repository)};

  VirtualContextControl context_control;
  Adapter adapter{context_control.factory()};

  void SetUp() final { adapter.setInterfaces(device_builder, device_registry); }
};

TEST_F(ModbusTechnologyAdapterImplementationTests, noBus) {
  EXPECT_EQ(adapter.start_called, 0);
  EXPECT_EQ(adapter.stop_called, 0);
  EXPECT_EQ(adapter.add_bus_called, 0);
  EXPECT_EQ(adapter.cancel_bus_called, 0);
  EXPECT_EQ(registration_called, 0);
  EXPECT_EQ(deregistration_called, 0);

  adapter.start();

  std::this_thread::sleep_for(long_time);

  adapter.stop();

  EXPECT_EQ(adapter.start_called, 1);
  EXPECT_EQ(adapter.stop_called, 1);
  EXPECT_EQ(adapter.add_bus_called, 0);
  EXPECT_EQ(adapter.cancel_bus_called, 0);
  EXPECT_EQ(registration_called, 0);
  EXPECT_EQ(deregistration_called, 0);
}

TEST_F(ModbusTechnologyAdapterImplementationTests, goodBus) {
  std::optional<ReadFunction> read_metric;

  context_control.setDevice(port_name, device_id,
      LibModbus::ReadableRegisterType::HoldingRegister, 0, Quality::PERFECT);

  EXPECT_EQ(adapter.start_called, 0);
  EXPECT_EQ(adapter.stop_called, 0);
  EXPECT_EQ(adapter.add_bus_called, 0);
  EXPECT_EQ(adapter.cancel_bus_called, 0);
  EXPECT_EQ(registration_called, 0);
  EXPECT_EQ(deregistration_called, 0);

  adapter.start_callback = [this]() {
    EXPECT_EQ(adapter.start_called, 1);
    EXPECT_EQ(adapter.stop_called, 0);
    EXPECT_EQ(adapter.add_bus_called, 0);
    EXPECT_EQ(adapter.cancel_bus_called, 0);
    EXPECT_EQ(registration_called, 0);
    EXPECT_EQ(deregistration_called, 0);
  };

  adapter.add_bus_callback = //
      [this](Config::Bus::NonemptyPtr const&, Config::Portname const&) {
        EXPECT_EQ(adapter.start_called, 1);
        EXPECT_EQ(adapter.stop_called, 0);
        EXPECT_EQ(adapter.add_bus_called, 1);
        EXPECT_EQ(adapter.cancel_bus_called, 0);
        EXPECT_EQ(registration_called, 0);
        EXPECT_EQ(deregistration_called, 0);
      };

  registration_callback = [this, &read_metric](ReadFunction const& metric) {
    EXPECT_EQ(adapter.start_called, 1);
    EXPECT_EQ(adapter.stop_called, 0);
    EXPECT_EQ(adapter.add_bus_called, 1);
    EXPECT_EQ(adapter.cancel_bus_called, 0);
    EXPECT_EQ(registration_called, 1);
    EXPECT_EQ(deregistration_called, 0);
    read_metric = metric;
  };

  adapter.stop_callback = [this]() {
    EXPECT_EQ(adapter.start_called, 1);
    EXPECT_EQ(adapter.stop_called, 1);
    EXPECT_EQ(adapter.add_bus_called, 1);
    EXPECT_EQ(adapter.cancel_bus_called, 0);
    EXPECT_EQ(registration_called, 1);
    EXPECT_EQ(deregistration_called, 0);
  };

  adapter.start();

  std::this_thread::sleep_for(long_time);

  if (read_metric.has_value()) {
    EXPECT_EQ(read_metric.value()(), 1);

    context_control.setDevice(port_name, device_id,
        LibModbus::ReadableRegisterType::HoldingRegister, 1, Quality::PERFECT);
    EXPECT_EQ(read_metric.value()(), 131075);
  } else {
    FAIL() << "Not registered!";
  }

  adapter.stop();

  EXPECT_EQ(adapter.start_called, 1);
  EXPECT_EQ(adapter.stop_called, 1);
  EXPECT_EQ(adapter.add_bus_called, 1);
  EXPECT_EQ(adapter.cancel_bus_called, 0);
  EXPECT_EQ(registration_called, 1);
  EXPECT_EQ(deregistration_called, 1);
}

TEST_F(ModbusTechnologyAdapterImplementationTests,
    busVanishesSometimeAfterRegistration) {

  std::optional<ReadFunction> read_metric;

  context_control.setDevice(port_name, device_id,
      LibModbus::ReadableRegisterType::HoldingRegister, 0, Quality::PERFECT);

  EXPECT_EQ(adapter.start_called, 0);
  EXPECT_EQ(adapter.stop_called, 0);
  EXPECT_EQ(adapter.add_bus_called, 0);
  EXPECT_EQ(adapter.cancel_bus_called, 0);
  EXPECT_EQ(registration_called, 0);
  EXPECT_EQ(deregistration_called, 0);

  adapter.start_callback = [this]() {
    EXPECT_EQ(adapter.start_called, 1);
    EXPECT_EQ(adapter.stop_called, 0);
    EXPECT_EQ(adapter.add_bus_called, 0);
    EXPECT_EQ(adapter.cancel_bus_called, 0);
    EXPECT_EQ(registration_called, 0);
    EXPECT_EQ(deregistration_called, 0);
  };

  adapter.add_bus_callback = //
      [this](Config::Bus::NonemptyPtr const&, Config::Portname const&) {
        EXPECT_EQ(adapter.start_called, 1);
        EXPECT_EQ(adapter.stop_called, 0);
        EXPECT_EQ(adapter.add_bus_called, 1);
        EXPECT_EQ(adapter.cancel_bus_called, 0);
        EXPECT_EQ(registration_called, 0);
        EXPECT_EQ(deregistration_called, 0);
      };

  registration_callback = [this, &read_metric](ReadFunction const& metric) {
    EXPECT_EQ(adapter.start_called, 1);
    EXPECT_EQ(adapter.stop_called, 0);
    EXPECT_EQ(adapter.add_bus_called, 1);
    EXPECT_EQ(adapter.cancel_bus_called, 0);
    EXPECT_EQ(registration_called, 1);
    EXPECT_EQ(deregistration_called, 0);
    read_metric = metric;
  };

  adapter.cancel_bus_callback = [this](Config::Portname const&) {
    EXPECT_EQ(adapter.start_called, 1);
    EXPECT_EQ(adapter.stop_called, 0);
    EXPECT_EQ(adapter.add_bus_called, 1);
    EXPECT_EQ(adapter.cancel_bus_called, 1);
    EXPECT_EQ(registration_called, 1);

    // We don't care in which order deregistration and `cancelBus` happen.
    EXPECT_GE(deregistration_called, 0);
    EXPECT_LE(deregistration_called, 1);
  };

  adapter.stop_callback = [this]() {
    EXPECT_EQ(adapter.start_called, 1);
    EXPECT_EQ(adapter.stop_called, 1);
    EXPECT_EQ(adapter.add_bus_called, 1);
    EXPECT_EQ(adapter.cancel_bus_called, 1);
    EXPECT_EQ(registration_called, 1);
    EXPECT_EQ(deregistration_called, 1);
  };

  adapter.start();

  std::this_thread::sleep_for(long_time);

  if (read_metric.has_value()) {
    EXPECT_EQ(read_metric.value()(), 1);

    context_control.reset();

    EXPECT_EQ(adapter.start_called, 1);
    EXPECT_EQ(adapter.stop_called, 0);
    EXPECT_EQ(adapter.add_bus_called, 1);
    EXPECT_EQ(adapter.cancel_bus_called, 0);
    EXPECT_EQ(registration_called, 1);
    EXPECT_EQ(deregistration_called, 0);

    EXPECT_THROW(read_metric.value()(), std::runtime_error);
  } else {
    FAIL() << "Not registered!";
  }

  EXPECT_EQ(adapter.start_called, 1);
  EXPECT_EQ(adapter.stop_called, 0);
  EXPECT_EQ(adapter.add_bus_called, 1);
  EXPECT_EQ(adapter.cancel_bus_called, 1);
  EXPECT_EQ(registration_called, 1);
  EXPECT_EQ(deregistration_called, 1);

  adapter.stop();

  EXPECT_EQ(adapter.start_called, 1);
  EXPECT_EQ(adapter.stop_called, 1);
  EXPECT_EQ(adapter.add_bus_called, 1);
  EXPECT_EQ(adapter.cancel_bus_called, 1);
  EXPECT_EQ(registration_called, 1);
  EXPECT_EQ(deregistration_called, 1);
}

TEST_F(ModbusTechnologyAdapterImplementationTests, busVanishesTemporarily) {
  size_t previous_buses = 0;
  std::optional<ReadFunction> read_metric;

  context_control.setDevice(port_name, device_id,
      LibModbus::ReadableRegisterType::HoldingRegister, 0, Quality::PERFECT);

  EXPECT_EQ(adapter.start_called, 0);
  EXPECT_EQ(adapter.stop_called, 0);
  EXPECT_EQ(adapter.add_bus_called, 0);
  EXPECT_EQ(adapter.cancel_bus_called, 0);
  EXPECT_EQ(registration_called, 0);
  EXPECT_EQ(deregistration_called, 0);

  adapter.start_callback = [this]() {
    EXPECT_EQ(adapter.start_called, 1);
    EXPECT_EQ(adapter.stop_called, 0);
    EXPECT_EQ(adapter.add_bus_called, 0);
    EXPECT_EQ(adapter.cancel_bus_called, 0);
    EXPECT_EQ(registration_called, 0);
    EXPECT_EQ(deregistration_called, 0);
  };

  adapter.add_bus_callback = //
      [this, &previous_buses](
          Config::Bus::NonemptyPtr const&, Config::Portname const&) {
        //
        EXPECT_EQ(adapter.start_called, 1);
        EXPECT_EQ(adapter.stop_called, 0);
        EXPECT_EQ(adapter.add_bus_called, 1 + previous_buses);
        EXPECT_EQ(adapter.cancel_bus_called, previous_buses);
        EXPECT_EQ(registration_called, previous_buses);
        EXPECT_EQ(deregistration_called, previous_buses);
      };

  registration_callback = //
      [this, &read_metric, &previous_buses](ReadFunction const& metric) {
        EXPECT_EQ(adapter.start_called, 1);
        EXPECT_EQ(adapter.stop_called, 0);
        EXPECT_EQ(adapter.add_bus_called, 1 + previous_buses);
        EXPECT_EQ(adapter.cancel_bus_called, previous_buses);
        EXPECT_EQ(registration_called, 1 + previous_buses);
        EXPECT_EQ(deregistration_called, previous_buses);
        read_metric = metric;
      };

  adapter.cancel_bus_callback = [this](Config::Portname const&) {
    EXPECT_EQ(adapter.start_called, 1);
    EXPECT_EQ(adapter.stop_called, 0);
    EXPECT_EQ(adapter.add_bus_called, 1);
    EXPECT_EQ(adapter.cancel_bus_called, 1);
    EXPECT_EQ(registration_called, 1);

    // We don't care in which order deregistration and `cancelBus` happen.
    EXPECT_GE(deregistration_called, 0);
    EXPECT_LE(deregistration_called, 1);
  };

  adapter.stop_callback = [this]() {
    EXPECT_EQ(adapter.start_called, 1);
    EXPECT_EQ(adapter.stop_called, 1);
    EXPECT_EQ(adapter.add_bus_called, 2);
    EXPECT_EQ(adapter.cancel_bus_called, 1);
    EXPECT_EQ(registration_called, 2);
    EXPECT_EQ(deregistration_called, 1);
  };

  adapter.start();

  std::this_thread::sleep_for(long_time);

  if (read_metric.has_value()) {
    EXPECT_EQ(read_metric.value()(), 1);

    context_control.reset();

    EXPECT_EQ(adapter.start_called, 1);
    EXPECT_EQ(adapter.stop_called, 0);
    EXPECT_EQ(adapter.add_bus_called, 1);
    EXPECT_EQ(adapter.cancel_bus_called, 0);
    EXPECT_EQ(registration_called, 1);
    EXPECT_EQ(deregistration_called, 0);

    EXPECT_THROW(read_metric.value()(), std::runtime_error);
  } else {
    FAIL() << "Not registered!";
  }

  // bring bus back
  ++previous_buses;
  context_control.setDevice(port_name, device_id,
      LibModbus::ReadableRegisterType::HoldingRegister, 1, Quality::PERFECT);
  std::this_thread::sleep_for(long_time);

  if (read_metric.has_value()) {
    EXPECT_EQ(read_metric.value()(), 131075);
  } else {
    FAIL() << "Not registered!";
  }

  EXPECT_EQ(adapter.start_called, 1);
  EXPECT_EQ(adapter.stop_called, 0);
  EXPECT_EQ(adapter.add_bus_called, 2);
  EXPECT_EQ(adapter.cancel_bus_called, 1);
  EXPECT_EQ(registration_called, 2);
  EXPECT_EQ(deregistration_called, 1);

  adapter.stop();

  EXPECT_EQ(adapter.start_called, 1);
  EXPECT_EQ(adapter.stop_called, 1);
  EXPECT_EQ(adapter.add_bus_called, 2);
  EXPECT_EQ(adapter.cancel_bus_called, 1);
  EXPECT_EQ(registration_called, 2);
  EXPECT_EQ(deregistration_called, 2);
}

TEST_F(ModbusTechnologyAdapterImplementationTests, busReappearsOnOtherPort) {
  size_t previous_buses = 0;
  std::optional<ReadFunction> read_metric;

  context_control.setDevice(port_name, device_id,
      LibModbus::ReadableRegisterType::HoldingRegister, 0, Quality::PERFECT);

  EXPECT_EQ(adapter.start_called, 0);
  EXPECT_EQ(adapter.stop_called, 0);
  EXPECT_EQ(adapter.add_bus_called, 0);
  EXPECT_EQ(adapter.cancel_bus_called, 0);
  EXPECT_EQ(registration_called, 0);
  EXPECT_EQ(deregistration_called, 0);

  adapter.start_callback = [this]() {
    EXPECT_EQ(adapter.start_called, 1);
    EXPECT_EQ(adapter.stop_called, 0);
    EXPECT_EQ(adapter.add_bus_called, 0);
    EXPECT_EQ(adapter.cancel_bus_called, 0);
    EXPECT_EQ(registration_called, 0);
    EXPECT_EQ(deregistration_called, 0);
  };

  adapter.add_bus_callback = //
      [this, &previous_buses](
          Config::Bus::NonemptyPtr const&, Config::Portname const&) {
        //
        EXPECT_EQ(adapter.start_called, 1);
        EXPECT_EQ(adapter.stop_called, 0);
        EXPECT_EQ(adapter.add_bus_called, 1 + previous_buses);
        EXPECT_EQ(adapter.cancel_bus_called, previous_buses);
        EXPECT_EQ(registration_called, previous_buses);
        EXPECT_EQ(deregistration_called, previous_buses);
      };

  registration_callback = //
      [this, &read_metric, &previous_buses](ReadFunction const& metric) {
        EXPECT_EQ(adapter.start_called, 1);
        EXPECT_EQ(adapter.stop_called, 0);
        EXPECT_EQ(adapter.add_bus_called, 1 + previous_buses);
        EXPECT_EQ(adapter.cancel_bus_called, previous_buses);
        EXPECT_EQ(registration_called, 1 + previous_buses);
        EXPECT_EQ(deregistration_called, previous_buses);
        read_metric = metric;
      };

  adapter.cancel_bus_callback = [this](Config::Portname const&) {
    EXPECT_EQ(adapter.start_called, 1);
    EXPECT_EQ(adapter.stop_called, 0);
    EXPECT_EQ(adapter.add_bus_called, 1);
    EXPECT_EQ(adapter.cancel_bus_called, 1);
    EXPECT_EQ(registration_called, 1);

    // We don't care in which order deregistration and `cancelBus` happen.
    EXPECT_GE(deregistration_called, 0);
    EXPECT_LE(deregistration_called, 1);
  };

  adapter.stop_callback = [this]() {
    EXPECT_EQ(adapter.start_called, 1);
    EXPECT_EQ(adapter.stop_called, 1);
    EXPECT_EQ(adapter.add_bus_called, 2);
    EXPECT_EQ(adapter.cancel_bus_called, 1);
    EXPECT_EQ(registration_called, 2);
    EXPECT_EQ(deregistration_called, 1);
  };

  adapter.start();

  std::this_thread::sleep_for(long_time);

  if (read_metric.has_value()) {
    EXPECT_EQ(read_metric.value()(), 1);

    context_control.reset();

    EXPECT_EQ(adapter.start_called, 1);
    EXPECT_EQ(adapter.stop_called, 0);
    EXPECT_EQ(adapter.add_bus_called, 1);
    EXPECT_EQ(adapter.cancel_bus_called, 0);
    EXPECT_EQ(registration_called, 1);
    EXPECT_EQ(deregistration_called, 0);

    EXPECT_THROW(read_metric.value()(), std::runtime_error);
  } else {
    FAIL() << "Not registered!";
  }

  // bring bus back, on other port
  ++previous_buses;
  context_control.setDevice(other_port_name, device_id,
      LibModbus::ReadableRegisterType::HoldingRegister, 1, Quality::PERFECT);
  std::this_thread::sleep_for(long_time);

  if (read_metric.has_value()) {
    EXPECT_EQ(read_metric.value()(), 131075);
  } else {
    FAIL() << "Not registered!";
  }

  EXPECT_EQ(adapter.start_called, 1);
  EXPECT_EQ(adapter.stop_called, 0);
  EXPECT_EQ(adapter.add_bus_called, 2);
  EXPECT_EQ(adapter.cancel_bus_called, 1);
  EXPECT_EQ(registration_called, 2);
  EXPECT_EQ(deregistration_called, 1);

  adapter.stop();

  EXPECT_EQ(adapter.start_called, 1);
  EXPECT_EQ(adapter.stop_called, 1);
  EXPECT_EQ(adapter.add_bus_called, 2);
  EXPECT_EQ(adapter.cancel_bus_called, 1);
  EXPECT_EQ(registration_called, 2);
  EXPECT_EQ(deregistration_called, 2);
}

TEST_F(
    ModbusTechnologyAdapterImplementationTests, busVanishesDuringRegistration) {

  std::optional<ReadFunction> read_metric;

  context_control.setDevice(port_name, device_id,
      LibModbus::ReadableRegisterType::HoldingRegister, 0, Quality::PERFECT);

  EXPECT_EQ(adapter.start_called, 0);
  EXPECT_EQ(adapter.stop_called, 0);
  EXPECT_EQ(adapter.add_bus_called, 0);
  EXPECT_EQ(adapter.cancel_bus_called, 0);
  EXPECT_EQ(registration_called, 0);
  EXPECT_EQ(deregistration_called, 0);

  adapter.start_callback = [this]() {
    EXPECT_EQ(adapter.start_called, 1);
    EXPECT_EQ(adapter.stop_called, 0);
    EXPECT_EQ(adapter.add_bus_called, 0);
    EXPECT_EQ(adapter.cancel_bus_called, 0);
    EXPECT_EQ(registration_called, 0);
    EXPECT_EQ(deregistration_called, 0);
  };

  adapter.add_bus_callback = //
      [this](Config::Bus::NonemptyPtr const&, Config::Portname const&) {
        EXPECT_EQ(adapter.start_called, 1);
        EXPECT_EQ(adapter.stop_called, 0);
        EXPECT_EQ(adapter.add_bus_called, 1);
        EXPECT_EQ(adapter.cancel_bus_called, 0);
        EXPECT_EQ(registration_called, 0);
        EXPECT_EQ(deregistration_called, 0);
      };

  registration_callback = [this, &read_metric](ReadFunction const& metric) {
    EXPECT_EQ(adapter.start_called, 1);
    EXPECT_EQ(adapter.stop_called, 0);
    EXPECT_EQ(adapter.add_bus_called, 1);
    EXPECT_EQ(adapter.cancel_bus_called, 0);
    EXPECT_EQ(registration_called, 1);
    EXPECT_EQ(deregistration_called, 0);

    read_metric = metric;
    context_control.reset();
    EXPECT_THROW(metric(), std::runtime_error);
  };

  adapter.cancel_bus_callback = [this](Config::Portname const&) {
    EXPECT_EQ(adapter.start_called, 1);
    EXPECT_EQ(adapter.stop_called, 0);
    EXPECT_EQ(adapter.add_bus_called, 1);
    EXPECT_EQ(adapter.cancel_bus_called, 1);
    EXPECT_EQ(registration_called, 1);

    // We don't care in which order deregistration and `cancelBus` happen.
    EXPECT_GE(deregistration_called, 0);
    EXPECT_LE(deregistration_called, 1);
  };

  adapter.stop_callback = [this]() {
    EXPECT_EQ(adapter.start_called, 1);
    EXPECT_EQ(adapter.stop_called, 1);
    EXPECT_EQ(adapter.add_bus_called, 1);
    EXPECT_EQ(adapter.cancel_bus_called, 1);
    EXPECT_EQ(registration_called, 1);
    EXPECT_EQ(deregistration_called, 1);
  };

  adapter.start();

  std::this_thread::sleep_for(long_time);

  EXPECT_EQ(adapter.start_called, 1);
  EXPECT_EQ(adapter.stop_called, 0);
  EXPECT_EQ(adapter.add_bus_called, 1);
  EXPECT_EQ(adapter.cancel_bus_called, 1);
  EXPECT_EQ(registration_called, 1);
  EXPECT_EQ(deregistration_called, 1);

  if (read_metric.has_value()) {
    EXPECT_THROW(read_metric.value()(), std::runtime_error);
  } else {
    FAIL() << "Never registered!";
  }

  EXPECT_EQ(adapter.start_called, 1);
  EXPECT_EQ(adapter.stop_called, 0);
  EXPECT_EQ(adapter.add_bus_called, 1);
  EXPECT_EQ(adapter.cancel_bus_called, 1);
  EXPECT_EQ(registration_called, 1);
  EXPECT_EQ(deregistration_called, 1);

  adapter.stop();

  EXPECT_EQ(adapter.start_called, 1);
  EXPECT_EQ(adapter.stop_called, 1);
  EXPECT_EQ(adapter.add_bus_called, 1);
  EXPECT_EQ(adapter.cancel_bus_called, 1);
  EXPECT_EQ(registration_called, 1);
  EXPECT_EQ(deregistration_called, 1);
}

// NOLINTEND(cert-err58-cpp, readability-magic-numbers)

} // namespace
  // ModbusTechnologyAdapterTests::ModbusTechnologyAdapterImplementationTests
