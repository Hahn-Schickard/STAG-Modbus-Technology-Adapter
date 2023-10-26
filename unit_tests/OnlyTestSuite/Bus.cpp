#include "gtest/gtest.h"

#include <Information_Model/mocks/DeviceMockBuilder.hpp>
#include <Technology_Adapter_Interface/TechnologyAdapterInterface.hpp>
#include <Technology_Adapter_Interface/mocks/ModelRepositoryInterface_MOCK.hpp>

#include "internal/Bus.hpp"
#include "internal/ConfigJson.hpp"

#include "VirtualAdapter.hpp"
#include "VirtualContext.hpp"

namespace ModbusTechnologyAdapterTests::BusTests {

using namespace Technology_Adapter::Modbus;
using namespace Virtual_Context;

// NOLINTBEGIN(cert-err58-cpp, readability-magic-numbers)

ConstString::ConstString device_name{"The device"};
ConstString::ConstString port_name{"The port"};

// clang-format off
auto bus_config = Config::BusOfJson({
  {"possible_serial_ports", {"The port"}},
  {"devices", {
    {
      {"slave_id", 10},
      {"id", "The device"},
      {"name", "N"},
      {"description", "D"},
      {"holding_registers", {
        {{"begin", 2}, {"end", 3}},
        {{"begin", 5}, {"end", 5}},
      }},
      {"input_registers", nlohmann::json::array()},
      {"burst_size", 1},
      {"elements", {
        {
          {"element_type", "readable"},
          {"name", "N1"},
          {"description", "D1"},
          {"registers", {3}},
          {"decoder", {
            {"type", "linear"},
            {"factor", 2},
            {"offset", 1},
          }},
        },
        {
          {"element_type", "group"},
          {"name", "N2"},
          {"description", "D2"},
          {"elements", {
            {
              {"element_type", "readable"},
              {"name", "N3"},
              {"description", "D3"},
              {"registers", {2, 5}},
              {"decoder", {
                {"type", "linear"},
                {"factor", 3},
                {"offset", 4},
              }},
            },
          }},
        },
      }},
    },
  }},
  {"baud", 1},
  {"parity", "None"},
  {"stop_bits", 2},
  {"data_bits", 3},
});
// clang-format on

struct BusTests : public testing::Test {
  VirtualAdapter::VirtualAdapter adapter;
  size_t registration_called = 0;
  size_t deregistration_called = 0;
  Information_Model::MetricPtr metric1;
  Information_Model::MetricPtr metric2;

  Technology_Adapter::testing::RegistrationHandler registration_handler =
      [this](Information_Model::NonemptyDevicePtr device) -> bool {
    //
    ++registration_called;
    EXPECT_EQ(device->getElementId(), "The device");
    auto elements = device->getDeviceElementGroup()->getSubelements();

    EXPECT_EQ(elements.size(), 2);
    size_t readable_index = //
        elements.at(0)->getElementType() ==
            Information_Model::ElementType::READABLE
        ? 0
        : 1;
    size_t subgroup_index = 1 - readable_index;

    auto readable1 = elements.at(readable_index);
    EXPECT_EQ(readable1->getElementName(), "N1");
    metric1 =
        std::get<Information_Model::NonemptyMetricPtr>(readable1->functionality)
            .base();

    auto subgroup = elements.at(subgroup_index);
    EXPECT_EQ(subgroup->getElementName(), "N2");
    auto subelements =
        std::get<Information_Model::NonemptyDeviceElementGroupPtr>(
            subgroup->functionality)
            .base()
            ->getSubelements();

    EXPECT_EQ(subelements.size(), 1);
    auto readable2 = subelements.at(0);
    EXPECT_EQ(readable2->getElementName(), "N3");
    metric2 =
        std::get<Information_Model::NonemptyMetricPtr>(readable2->functionality)
            .base();

    return true;
  };

  Technology_Adapter::testing::DeregistrationHandler deregistration_handler =
      [this](const std::string&) -> bool {
    //
    ++deregistration_called;
    return true;
  };

  Technology_Adapter::NonemptyModelRepositoryInterfacePtr const repository{
      std::make_shared<
          testing::NiceMock<Technology_Adapter::testing::ModelRepositoryMock>>(
          registration_handler, deregistration_handler)};

  Technology_Adapter::NonemptyDeviceRegistryPtr const registry{
      std::make_shared<Technology_Adapter::DeviceRegistry>(repository)};

  Information_Model::NonemptyDeviceBuilderInterfacePtr const builder{
      std::make_shared<Information_Model::testing::DeviceMockBuilder>()};

  void SetUp() final { VirtualContext::reset(); }

  void TearDown() final {
    // `Bus` has no reason ever to call anything except `cancelBus`
    EXPECT_EQ(adapter.start_called, 0);
    EXPECT_EQ(adapter.stop_called, 0);
    EXPECT_EQ(adapter.add_bus_called, 0);
  }

  void readOften() {
    for (int i = 0; i < 100; ++i) {
      metric1->getMetricValue();
      metric2->getMetricValue();
    }
  }
};

TEST_F(BusTests, buildModel) {
  auto bus = Bus::NonemptyPtr::make(
      adapter, bus_config, VirtualContext::make, port_name, registry);

  EXPECT_EQ(registration_called, 0);
  bus->buildModel(builder);
  EXPECT_EQ(registration_called, 1);
  EXPECT_EQ(deregistration_called, 0);
  EXPECT_EQ(adapter.cancel_bus_called, 0);
}

TEST_F(BusTests, getMetricValue) {
  auto bus = Bus::NonemptyPtr::make(
      adapter, bus_config, VirtualContext::make, port_name, registry);
  bus->buildModel(builder);
  bus->start();

  VirtualContext::setDevice(device_name,
      LibModbus::ReadableRegisterType::HoldingRegister, 1, Quality::PERFECT);
  EXPECT_EQ(std::get<double>(metric1->getMetricValue()), 3);
  EXPECT_EQ(std::get<double>(metric2->getMetricValue()), 3 * 65537 + 4);

  VirtualContext::setDevice(device_name,
      LibModbus::ReadableRegisterType::HoldingRegister, 2, Quality::PERFECT);
  EXPECT_EQ(std::get<double>(metric1->getMetricValue()), 5);
  EXPECT_EQ(std::get<double>(metric2->getMetricValue()), 6 * 65537 + 4);

  EXPECT_EQ(registration_called, 1);
  EXPECT_EQ(deregistration_called, 0);
  EXPECT_EQ(adapter.cancel_bus_called, 0);
}

TEST_F(BusTests, shutDownOnMissingDevice) {
  auto bus = Bus::NonemptyPtr::make(
      adapter, bus_config, VirtualContext::make, port_name, registry);
  bus->buildModel(builder);
  bus->start();

  EXPECT_THROW(readOften(), std::runtime_error);

  EXPECT_EQ(registration_called, 1);
  EXPECT_EQ(deregistration_called, 1);
  EXPECT_EQ(adapter.cancel_bus_called, 1);
}

TEST_F(BusTests, shutDownOnUnreliableDevice) {
  auto bus = Bus::NonemptyPtr::make(
      adapter, bus_config, VirtualContext::make, port_name, registry);
  bus->buildModel(builder);
  bus->start();

  VirtualContext::setDevice(device_name,
      LibModbus::ReadableRegisterType::HoldingRegister, 0, Quality::UNRELIABLE);

  EXPECT_THROW(readOften(), std::runtime_error);

  EXPECT_EQ(registration_called, 1);
  EXPECT_EQ(deregistration_called, 1);
  EXPECT_EQ(adapter.cancel_bus_called, 1);
}

TEST_F(BusTests, shutDownOnNoisyDevice) {
  auto bus = Bus::NonemptyPtr::make(
      adapter, bus_config, VirtualContext::make, port_name, registry);
  bus->buildModel(builder);
  bus->start();

  VirtualContext::setDevice(device_name,
      LibModbus::ReadableRegisterType::HoldingRegister, 0, Quality::NOISY);

  EXPECT_THROW(readOften(), std::runtime_error);

  EXPECT_EQ(registration_called, 1);
  EXPECT_EQ(deregistration_called, 1);
  EXPECT_EQ(adapter.cancel_bus_called, 1);
}

// NOLINTEND(cert-err58-cpp, readability-magic-numbers))

} // namespace ModbusTechnologyAdapterTests::BusTests
