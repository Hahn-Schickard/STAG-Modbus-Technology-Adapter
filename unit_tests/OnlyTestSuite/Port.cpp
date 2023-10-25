#include <chrono>

#include "gtest/gtest.h"

#include "HaSLL/LoggerManager.hpp"
#include "HaSLL/SPD_LoggerRepository.hpp"

#include "internal/LibmodbusAbstraction.hpp"
#include "internal/Port.hpp"
#include "internal/PortFinderPlan.hpp"

#include "Specs.hpp"
#include "VirtualContext.hpp"

namespace ModbusTechnologyAdapterTests::PortTests {

using namespace Technology_Adapter::Modbus;
using namespace SpecsForTests;
using namespace Virtual_Context;

// NOLINTBEGIN(cert-err58-cpp, readability-magic-numbers)

auto long_time = std::chrono::milliseconds(100);

ConstString::ConstString device_id{"The device"};
ConstString::ConstString port_name{"The port"};

PortFinderPlan::Candidate candidate( //
    std::vector<DeviceSpec>&& devices,
    ConstString::ConstString const& expected_bus_id,
    ConstString::ConstString const& port) {
  /*
    As `Candidate`s are only created by `PortFinderPlan`, we tweak an instance
    of the latter so that it emits a `Candidate` as specified.
    The methods `stillFeasible` and `confirm` are not meaningful as they refer
    to a throw-away `PortFinderPlan`
  */
  auto plan = PortFinderPlan::make();
  auto bus = specToConfig(BusSpec({port}, std::move(devices)));

  auto candidates = plan->addBuses({bus});

  EXPECT_EQ(candidates.size(), 1);
  auto const& candidate = candidates.at(0);
  EXPECT_EQ(candidate.getBus()->id, expected_bus_id);
  EXPECT_EQ(candidate.getPort(), port);

  return candidate;
}

struct PortTests : public testing::Test {
  void SetUp() final { VirtualContext::reset(); }
};

TEST_F(PortTests, findsDevice) {
  bool found = false;

  auto success_callback = [&found](PortFinderPlan::Candidate const& candidate) {
    found = true;
    EXPECT_EQ(candidate.getBus()->id, device_id);
    EXPECT_EQ(candidate.getPort(), port_name);
  };

  VirtualContext::setDevice(device_id,
      LibModbus::ReadableRegisterType::HoldingRegister, 0, Quality::PERFECT);

  Port port(VirtualContext::make, port_name, success_callback);
  port.addCandidate(candidate( //
      std::vector<DeviceSpec>{{device_id, 10, {{2, 3}, {5, 5}}, {}}},
      device_id, port_name));

  std::this_thread::sleep_for(long_time);

  port.stop();
  EXPECT_TRUE(found);
}

TEST_F(PortTests, rejectsWrongRegisterType) {
  bool found = false;

  auto success_callback = [&found](PortFinderPlan::Candidate const&) {
    found = true;
  };

  VirtualContext::setDevice(device_id,
      LibModbus::ReadableRegisterType::HoldingRegister, 0, Quality::PERFECT);

  Port port(VirtualContext::make, port_name, success_callback);
  port.addCandidate(candidate( //
      std::vector<DeviceSpec>{{device_id, 10, {}, {{2, 3}, {5, 5}}}},
      device_id, port_name));

  std::this_thread::sleep_for(long_time);

  port.stop();
  EXPECT_FALSE(found);
}

TEST_F(PortTests, rejectsExtraRegisters) {
  bool found = false;

  auto success_callback = [&found](PortFinderPlan::Candidate const&) {
    found = true;
  };

  VirtualContext::setDevice(device_id,
      LibModbus::ReadableRegisterType::HoldingRegister, 0, Quality::PERFECT);

  Port port(VirtualContext::make, port_name, success_callback);
  port.addCandidate(
      candidate({{device_id, 10, {{2, 5}}, {}}}, device_id, port_name));

  std::this_thread::sleep_for(long_time);

  port.stop();
  EXPECT_FALSE(found);
}

TEST_F(PortTests, findsAmongFailing) {
  // We use all candidates from the previous tests

  bool found = false;

  auto success_callback = [&found](PortFinderPlan::Candidate const& candidate) {
    found = true;
    EXPECT_EQ(candidate.getBus()->id, device_id);
    EXPECT_EQ(candidate.getPort(), port_name);
  };

  VirtualContext::setDevice(device_id,
      LibModbus::ReadableRegisterType::HoldingRegister, 0, Quality::PERFECT);

  Port port(VirtualContext::make, port_name, success_callback);
  port.addCandidate(candidate( //
      {{device_id, 10, {}, {{2, 3}, {5, 5}}}}, device_id, port_name));
  port.addCandidate(
      candidate({{device_id, 10, {{2, 5}}, {}}}, device_id, port_name));
  // last, the one that should succeed
  port.addCandidate(candidate( //
      {{device_id, 10, {{2, 3}, {5, 5}}, {}}}, device_id, port_name));

  std::this_thread::sleep_for(long_time);

  port.stop();
  EXPECT_TRUE(found);
}

TEST_F(PortTests, findsUnreliableDeviceEventually) {
  bool found = false;

  auto success_callback = [&found](PortFinderPlan::Candidate const& candidate) {
    found = true;
    EXPECT_EQ(candidate.getBus()->id, device_id);
    EXPECT_EQ(candidate.getPort(), port_name);
  };

  VirtualContext::setDevice(device_id,
      LibModbus::ReadableRegisterType::InputRegister, 0, Quality::UNRELIABLE);

  Port port(VirtualContext::make, port_name, success_callback);
  port.addCandidate(
      candidate({{device_id, 10, {}, {{2, 3}, {5, 5}}}}, device_id, port_name));

  std::this_thread::sleep_for(long_time);

  port.stop();
  EXPECT_TRUE(found);
}

TEST_F(PortTests, findsNoisyDeviceEventually) {
  bool found = false;

  auto success_callback = [&found](PortFinderPlan::Candidate const& candidate) {
    found = true;
    EXPECT_EQ(candidate.getBus()->id, device_id);
    EXPECT_EQ(candidate.getPort(), port_name);
  };

  VirtualContext::setDevice(device_id,
      LibModbus::ReadableRegisterType::InputRegister, 0, Quality::NOISY);

  Port port(VirtualContext::make, port_name, success_callback);
  port.addCandidate(
      candidate({{device_id, 10, {}, {{2, 3}, {5, 5}}}}, device_id, port_name));

  std::this_thread::sleep_for(long_time);

  port.stop();
  EXPECT_TRUE(found);
}

TEST_F(PortTests, findsRepeatedly) {
  size_t found = 0;

  auto success_callback = [&found](PortFinderPlan::Candidate const& candidate) {
    ++found;
    EXPECT_EQ(candidate.getBus()->id, device_id);
    EXPECT_EQ(candidate.getPort(), port_name);
  };

  VirtualContext::setDevice(device_id,
      LibModbus::ReadableRegisterType::HoldingRegister, 0, Quality::PERFECT);

  Port port(VirtualContext::make, port_name, success_callback);

  for (size_t i = 1; i < 5; ++i) {
    port.addCandidate(candidate(
        {{device_id, 10, {{2, 3}, {5, 5}}, {}}}, device_id, port_name));

    std::this_thread::sleep_for(long_time);

    EXPECT_EQ(found, i);
    port.reset();
  }

  port.stop();
}

// NOLINTEND(cert-err58-cpp, readability-magic-numbers))

} // namespace ModbusTechnologyAdapterTests::PortTests
