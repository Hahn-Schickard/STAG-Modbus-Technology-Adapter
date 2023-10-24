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
using namespace VirtualContext;

// NOLINTBEGIN(cert-err58-cpp, readability-magic-numbers)

auto long_time = std::chrono::milliseconds(100);

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

TEST(PortTests, findsDevice) {
  bool found = false;

  auto success_callback = [&found](PortFinderPlan::Candidate const& candidate) {
    found = true;
    EXPECT_EQ(candidate.getBus()->id, device1_name);
    EXPECT_EQ(candidate.getPort(), port_name);
  };

  Port port(VirtualContext::VirtualContext::make, port_name, success_callback);
  port.addCandidate(candidate(
      std::vector<DeviceSpec>{{device1_name, 10, {{2, 3}, {5, 5}}, {}}},
      device1_name, port_name));

  std::this_thread::sleep_for(long_time);

  port.stop();
  EXPECT_TRUE(found);
}

TEST(PortTests, rejectsWrongRegisterType) {
  bool found = false;

  auto success_callback = [&found](PortFinderPlan::Candidate const&) {
    found = true;
  };

  Port port(VirtualContext::VirtualContext::make, port_name, success_callback);
  port.addCandidate(candidate(
      std::vector<DeviceSpec>{{device1_name, 10, {}, {{2, 3}, {5, 5}}}},
      device1_name, port_name));

  std::this_thread::sleep_for(long_time);

  port.stop();
  EXPECT_FALSE(found);
}

TEST(PortTests, rejectsExtraRegisters) {
  bool found = false;

  auto success_callback = [&found](PortFinderPlan::Candidate const&) {
    found = true;
  };

  Port port(VirtualContext::VirtualContext::make, port_name, success_callback);
  port.addCandidate(candidate(
      {{device1_name, 10, {{2, 5}}, {}}}, device1_name, port_name));

  std::this_thread::sleep_for(long_time);

  port.stop();
  EXPECT_FALSE(found);
}

TEST(PortTests, findsAmongFailing) {
  // We use all candidates from the previous tests

  bool found = false;

  auto success_callback = [&found](PortFinderPlan::Candidate const& candidate) {
    found = true;
    EXPECT_EQ(candidate.getBus()->id, device1_name);
    EXPECT_EQ(candidate.getPort(), port_name);
  };

  Port port(VirtualContext::VirtualContext::make, port_name, success_callback);
  port.addCandidate(candidate(
      {{device1_name, 10, {}, {{2, 3}, {5, 5}}}}, device1_name, port_name));
  port.addCandidate(candidate(
      {{device1_name, 10, {{2, 5}}, {}}}, device1_name, port_name));
  // last, the one that should succeed
  port.addCandidate(candidate(
      {{device1_name, 10, {{2, 3}, {5, 5}}, {}}}, device1_name, port_name));

  std::this_thread::sleep_for(long_time);

  port.stop();
  EXPECT_TRUE(found);
}

TEST(PortTests, findsUnreliableDeviceEventually) {
  bool found = false;

  auto success_callback = [&found](PortFinderPlan::Candidate const& candidate) {
    found = true;
    EXPECT_EQ(candidate.getBus()->id, device2_name);
    EXPECT_EQ(candidate.getPort(), port_name);
  };

  Port port(VirtualContext::VirtualContext::make, port_name, success_callback);
  port.addCandidate(candidate(
      {{device2_name, 10, {}, {{2, 3}, {5, 5}}}}, device2_name, port_name));

  std::this_thread::sleep_for(long_time);

  port.stop();
  EXPECT_TRUE(found);
}

TEST(PortTests, findsNoisyDeviceEventually) {
  bool found = false;

  auto success_callback = [&found](PortFinderPlan::Candidate const& candidate) {
    found = true;
    EXPECT_EQ(candidate.getBus()->id, device3_name);
    EXPECT_EQ(candidate.getPort(), port_name);
  };

  Port port(VirtualContext::VirtualContext::make, port_name, success_callback);
  port.addCandidate(candidate(
      {{device3_name, 10, {}, {{2, 3}, {5, 5}}}}, device3_name, port_name));

  std::this_thread::sleep_for(long_time);

  port.stop();
  EXPECT_TRUE(found);
}

TEST(PortTests, findsRepeatedly) {
  size_t found = 0;

  auto success_callback = [&found](PortFinderPlan::Candidate const& candidate) {
    ++found;
    EXPECT_EQ(candidate.getBus()->id, device1_name);
    EXPECT_EQ(candidate.getPort(), port_name);
  };

  Port port(VirtualContext::VirtualContext::make, port_name, success_callback);

  for (size_t i = 1; i < 5; ++i) {
    port.addCandidate(candidate(
        {{device1_name, 10, {{2,3 }, {5, 5}}, {}}}, device1_name, port_name));

    std::this_thread::sleep_for(long_time);

    EXPECT_EQ(found, i);
    port.reset();
  }

  port.stop();
}

// NOLINTEND(cert-err58-cpp, readability-magic-numbers))

} // namespace ModbusTechnologyAdapterTests::PortTests
