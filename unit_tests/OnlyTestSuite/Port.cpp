#include <chrono>
#include <random>

#include "gtest/gtest.h"

#include "HaSLL/LoggerManager.hpp"
#include "HaSLL/SPD_LoggerRepository.hpp"

#include "internal/LibmodbusAbstraction.hpp"
#include "internal/Port.hpp"
#include "internal/PortFinderPlan.hpp"

#include "Specs.hpp"

namespace PortTests {

using namespace Technology_Adapter::Modbus;
using namespace SpecsForTests;

// NOLINTBEGIN(cert-err58-cpp, readability-magic-numbers)

auto long_time = std::chrono::milliseconds(100);

ConstString::ConstString port_name{"The port"};
ConstString::ConstString device1_name{"Device 1"};
ConstString::ConstString device2_name{"Device 2"};
ConstString::ConstString device3_name{"Device 3"};

std::minstd_rand random; // NOLINT(cert-msc32-c, cert-msc51-cpp)
std::uniform_int_distribution<> noise{0, 1};

[[noreturn]] void throwModbus(int errnum) {
  errno = errnum;
  throw LibModbus::ModbusError();
}

PortFinderPlan::Candidate candidate(
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

/*
  Hardcoded behaviour:
  - Device 1 has holding registers 2,3,5
  - Device 2 has input registers 3,5,7 and responds only with probability 1/2
  - Device 3 has input registers 3,5,7 and messes up CRC with probability 1/2
*/
class VirtualContext : public LibModbus::Context {
public:
  using Ptr = std::shared_ptr<VirtualContext>;

  void connect() override { connected_ = true; }
  void close() noexcept override { connected_ = false; }

  void selectDevice(
      Technology_Adapter::Modbus::Config::Device const& device) override {

    selected_device_ = device.id;
  }

  int readRegisters(int addr, LibModbus::ReadableRegisterType type, int nb,
      uint16_t*) override {

    if (selected_device_ == device1_name) {
      if (type != LibModbus::ReadableRegisterType::HoldingRegister) {
        throwModbus(LibModbus::ModbusError::XILADD);
      }

      return readRegisters(addr, nb);

    } else if (selected_device_ == device2_name) {
      if (type != LibModbus::ReadableRegisterType::InputRegister) {
        throwModbus(LibModbus::ModbusError::XILADD);
      }
      if (noise(random) == 1) {
        throwModbus(ETIMEDOUT);
      }

      return readRegisters(addr, nb);

    } else if (selected_device_ == device3_name) {
      if (type != LibModbus::ReadableRegisterType::InputRegister) {
        throwModbus(LibModbus::ModbusError::XILADD);
      }
      if (noise(random) == 1) {
        throwModbus(LibModbus::ModbusError::BADCRC);
      }

      return readRegisters(addr, nb);

    } else {
      // The selected device does not exist, so will not respond
      throwModbus(ETIMEDOUT);
    }
  }

  // a `Factory`
  static Ptr make(ConstString::ConstString const& /*port*/,
      Technology_Adapter::Modbus::Config::Bus const&) {

    return std::make_shared<VirtualContext>();
  }

private:
  int readRegisters(int addr, int nb) {
    switch (addr) {
    case 3:
    case 5:
      if (nb > 1) {
        throwModbus(LibModbus::ModbusError::MDATA);
      } else {
        return nb;
      }
    case 2:
      if (nb > 2) {
        throwModbus(LibModbus::ModbusError::MDATA);
      } else {
        return nb;
      }
    default:
      throwModbus(LibModbus::ModbusError::XILADD);
    }
  }

  bool connected_ = false;
  ConstString::ConstString selected_device_;
};

TEST(PortTests, findsDevice) {
  bool found = false;

  auto success_callback = [&found](PortFinderPlan::Candidate const& candidate) {
    found = true;
    EXPECT_EQ(candidate.getBus()->id, device1_name);
    EXPECT_EQ(candidate.getPort(), port_name);
  };

  Port port(VirtualContext::make, port_name, success_callback);
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

  Port port(VirtualContext::make, port_name, success_callback);
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

  Port port(VirtualContext::make, port_name, success_callback);
  port.addCandidate(candidate(
      std::vector<DeviceSpec>{{device1_name, 10, {{2, 5}}, {}}},
      device1_name, port_name));

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

  Port port(VirtualContext::make, port_name, success_callback);
  port.addCandidate(candidate(
      std::vector<DeviceSpec>{{device1_name, 10, {}, {{2, 3}, {5, 5}}}},
      device1_name, port_name));
  port.addCandidate(candidate(
      std::vector<DeviceSpec>{{device1_name, 10, {{2, 5}}, {}}},
      device1_name, port_name));
  // last, the one that should succeed
  port.addCandidate(candidate(
      std::vector<DeviceSpec>{{device1_name, 10, {{2, 3}, {5, 5}}, {}}},
      device1_name, port_name));

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

  Port port(VirtualContext::make, port_name, success_callback);
  port.addCandidate(candidate(
      std::vector<DeviceSpec>{{device2_name, 10, {}, {{2, 3}, {5, 5}}}},
      device2_name, port_name));

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

  Port port(VirtualContext::make, port_name, success_callback);
  port.addCandidate(candidate(
      std::vector<DeviceSpec>{{device3_name, 10, {}, {{2, 3}, {5, 5}}}},
      device3_name, port_name));

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

  Port port(VirtualContext::make, port_name, success_callback);

  for (size_t i = 1; i < 5; ++i) {
    port.addCandidate(candidate(
        std::vector<DeviceSpec>{{device1_name, 10, {{2,3 }, {5, 5}}, {}}},
        device1_name, port_name));

    std::this_thread::sleep_for(long_time);

    EXPECT_EQ(found, i);
    port.reset();
  }

  port.stop();
}

// NOLINTEND(cert-err58-cpp, readability-magic-numbers))

} // namespace PortTests
