#include "gtest/gtest.h"

#include <Information_Model/mocks/DeviceMockBuilder.hpp>
#include <Technology_Adapter_Interface/mocks/ModelRepositoryInterface_MOCK.hpp>
#include <Technology_Adapter_Interface/TechnologyAdapterInterface.hpp>

#include "internal/Bus.hpp"
#include "internal/ConfigJson.hpp"

#include "VirtualAdapter.hpp"
#include "VirtualContext.hpp"

namespace ModbusTechnologyAdapterTests::BusTests {

ConstString::ConstString port_name{"The port"};

auto bus_config = Technology_Adapter::Modbus::Config::BusOfJson({
        {"possible_serial_ports", {"The port"}},
        {"devices", {
            {
                {"slave_id", 10},
                {"id", "I"},
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

struct BusTests : public testing::Test {
  VirtualAdapter::VirtualAdapter adapter;
  size_t registration_called = 0;
  Information_Model::MetricPtr metric1;
  Information_Model::MetricPtr metric2;

  Technology_Adapter::testing::RegistrationHandler registration_handler =
      [this](Information_Model::NonemptyDevicePtr device) -> bool {
        ++registration_called;
        EXPECT_EQ(device->getElementId(), "I");
        auto elements = device->getDeviceElementGroup()->getSubelements();

        EXPECT_EQ(elements.size(), 2);
        size_t readable_index =
            elements.at(0)->getElementType() == Information_Model::ElementType::READABLE
                ? 0
                : 1;
        size_t subgroup_index = 1 - readable_index;

        auto readable1 = elements.at(readable_index);
        EXPECT_EQ(readable1->getElementName(), "N1");
        metric1 = std::get<Information_Model::NonemptyMetricPtr>(
            readable1->functionality).base();

        auto subgroup = elements.at(subgroup_index);
        EXPECT_EQ(subgroup->getElementName(), "N2");
        auto subelements = std::get<Information_Model::NonemptyDeviceElementGroupPtr>(
            subgroup->functionality).base()->getSubelements();

        EXPECT_EQ(subelements.size(), 1);
        auto readable2 = subelements.at(0);
        EXPECT_EQ(readable2->getElementName(), "N3");
        metric2 = std::get<Information_Model::NonemptyMetricPtr>(
            readable2->functionality).base();

        return true;
      };

  Technology_Adapter::testing::DeregistrationHandler deregistration_handler =
      [](const std::string&) -> bool {
        throw std::runtime_error("Deregistration handler called");
      };

  Technology_Adapter::NonemptyModelRepositoryInterfacePtr const repository{
      std::make_shared<testing::NiceMock<Technology_Adapter::testing::ModelRepositoryMock>>(
          registration_handler, deregistration_handler)};

  Technology_Adapter::NonemptyDeviceRegistryPtr const registry{
      std::make_shared<Technology_Adapter::DeviceRegistry>(repository)};

  Information_Model::NonemptyDeviceBuilderInterfacePtr const builder{
      std::make_shared<Information_Model::testing::DeviceMockBuilder>()};

  void TearDown() final {
    // `Bus` has no reason ever to call anything except `cancelBus`
    EXPECT_EQ(adapter.start_called, 0);
    EXPECT_EQ(adapter.stop_called, 0);
    EXPECT_EQ(adapter.add_bus_called, 0);
  }
};

TEST_F(BusTests, buildModel) {
  auto bus = Technology_Adapter::Modbus::Bus::NonemptyPtr::make(adapter,
      *bus_config, VirtualContext::VirtualContext::make, port_name, registry);

  EXPECT_EQ(registration_called, 0);
  bus->buildModel(builder);
  EXPECT_EQ(registration_called, 1);
}

} // namespace ModbusTechnologyAdapterTests::BusTests
