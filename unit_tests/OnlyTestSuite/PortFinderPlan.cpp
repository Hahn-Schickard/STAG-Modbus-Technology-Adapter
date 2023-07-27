#include "PortFinderPlan.hpp"

#include "gtest/gtest.h"

namespace PortFinderPlanTests {

// NOLINTBEGIN(readability-magic-numbers)

using namespace Technology_Adapter::Modbus;

struct DeviceSpec {
  using Registers = std::vector<RegisterRange>;

  std::string id; // should be unique throughout a test
  int slave_id;
  Registers holding_registers;
  Registers input_registers;

  DeviceSpec() = delete;
  // NOLINTNEXTLINE(readability-identifier-naming)
  DeviceSpec(std::string id_, int slave_id_, //
      Registers holding_registers_, Registers input_registers_)
      : id(std::move(id_)), slave_id(slave_id_),
        holding_registers(std::move(holding_registers_)),
        input_registers(std::move(input_registers_)) {}
};

struct BusSpec {
  using Ports = std::vector<std::string>;
  using Devices = std::vector<DeviceSpec>;

  Ports possible_ports;
  Devices devices;

  BusSpec() = delete;
  // NOLINTNEXTLINE(readability-identifier-naming)
  BusSpec(Ports&& possible_ports_, Devices&& devices_)
      : possible_ports(std::move(possible_ports_)),
        devices(std::move(devices_)) {}
};

struct CandidateSpec {
  std::string some_device_id_on_bus;
  std::string port;

  CandidateSpec() = delete;
  // NOLINTNEXTLINE(readability-identifier-naming)
  CandidateSpec(std::string&& device_id, std::string&& port_)
      : some_device_id_on_bus(std::move(device_id)), port(std::move(port_)) {}
};

struct PortFinderPlanTests : public testing::Test {

  PortFinderPlan::NonemptyPtr plan = PortFinderPlan::make();

  /*
    `Candidate`s are returned in the order given by `expected_new_candidates`.

    The returned candidates are tested for `stillFeasible`.
  */
  PortFinderPlan::NewCandidates addBuses(std::vector<BusSpec>&& bus_specs,
      std::vector<CandidateSpec>&& expected_new_candidates) {

    std::vector<Config::Bus::NonemptyPtr> buses;
    // translate `bus_spec` into `buses`
    for (auto& bus_spec : bus_specs) {
      auto bus = Threadsafe::SharedPtr<Config::Bus>::make( // not `const`
          bus_spec.possible_ports, 9600, LibModbus::Parity::None, 8, 2);
      for (auto& device : bus_spec.devices) {
        bus->devices.emplace_back(device.id, device.id /* as `name` */,
            device.id /* as `description` */, device.slave_id,
            1 /* as `burst_size` */, //
            std::move(device.holding_registers),
            std::move(device.input_registers));
      }
      buses.emplace_back(std::move(bus));
    }

    return checkAndSortNewCandidates(
        plan->addBuses(buses), std::move(expected_new_candidates));
  }

  PortFinderPlan::NewCandidates confirm(PortFinderPlan::Candidate& candidate,
      std::vector<CandidateSpec>&& expected_new_candidates) {

    EXPECT_TRUE(candidate.stillFeasible());
    auto actual_new_candidates = candidate.confirm();

    return checkAndSortNewCandidates(
        std::move(actual_new_candidates), std::move(expected_new_candidates));
  }

  PortFinderPlan::NewCandidates unassign(Config::Portname&& port,
      std::vector<CandidateSpec>&& expected_new_candidates) {

    auto actual_new_candidates = plan->unassign(port);

    return checkAndSortNewCandidates(
        std::move(actual_new_candidates), std::move(expected_new_candidates));
  }

  static void checkFeasibility(PortFinderPlan::NewCandidates const& candidates,
      std::vector<bool> const& expected_feasibilities) {

    EXPECT_EQ(candidates.size(), expected_feasibilities.size());
    for (size_t i = 0; i < candidates.size(); ++i) {
      EXPECT_EQ(candidates[i].stillFeasible(), expected_feasibilities[i]) << i;
    }
  }

private:
  static PortFinderPlan::NewCandidates checkAndSortNewCandidates(
      PortFinderPlan::NewCandidates&& inbound_new_candidates,
      std::vector<CandidateSpec>&& expected_new_candidates) {

    PortFinderPlan::NewCandidates outbound_new_candidates;

    EXPECT_EQ(inbound_new_candidates.size(), expected_new_candidates.size());
    // Check if we can find each expected candidate
    for (auto const& candidate_spec : expected_new_candidates) {
      auto candidate = std::find_if( //
          inbound_new_candidates.cbegin(), inbound_new_candidates.cend(),
          [&](PortFinderPlan::Candidate const& candidate) {
            Config::Bus::NonemptyPtr const& bus = candidate.getBus();
            return (candidate.getPort() == candidate_spec.port) &&
                std::any_of(bus->devices.cbegin(), bus->devices.cend(),
                    [&](Config::Device const& device) {
                      return device.id == candidate_spec.some_device_id_on_bus;
                    });
          });
      if (candidate == inbound_new_candidates.cend()) {
        ADD_FAILURE() //
            << candidate_spec.some_device_id_on_bus << " @ "
            << candidate_spec.port;
      } else {
        EXPECT_TRUE(candidate->stillFeasible());
        outbound_new_candidates.push_back(*candidate);
      }
    }

    return outbound_new_candidates;
  }
};

// NOLINTBEGIN(cert-err58-cpp)

TEST_F(PortFinderPlanTests, singleBusSinglePort) {
  auto candidates = addBuses(
      {
          {
              {"port 1"},
              {{"device 1", 1, {{1, 1}}, {}}},
          },
      },
      {{"device 1", "port 1"}});

  confirm(candidates.at(0), {});
  checkFeasibility(candidates, {false});
}

/*
  Even though each bus has a unique port, there must be no candidate:
  We allow buses not to be plugged.
*/
TEST_F(PortFinderPlanTests, twoBusesSinglePort) {
  addBuses(
      {
          {
              {"port 1"},
              {{"device 1", 1, {{1, 1}}, {}}},
          },
          {
              {"port 1"},
              {{"device 2", 1, {{1, 1}}, {}}},
          },
      },
      {});
}

TEST_F(PortFinderPlanTests, singleBusMultiplePorts) {
  auto candidates = addBuses(
      {
          {
              {"port 1", "port 2", "port 3"},
              {{"device 1", 1, {{1, 1}}, {}}},
          },
      },
      {{"device 1", "port 2"}, {"device 1", "port 3"}, {"device 1", "port 1"}});

  confirm(candidates.at(1), {});
  checkFeasibility(candidates, {false, false, false});
}

TEST_F(PortFinderPlanTests, twoBusesDisjointPorts) {
  auto candidates = addBuses(
      {
          {
              {"port 1", "port 2"},
              {{"device 1", 1, {{1, 1}}, {}}},
          },
          {
              {"port 3", "port 4"},
              {{"device 2", 2, {{1, 1}}, {}}},
          },
      },
      {
          {"device 1", "port 1"},
          {"device 1", "port 2"},

          {"device 2", "port 3"},
          {"device 2", "port 3"},
      });

  confirm(candidates.at(3), {});
  checkFeasibility(candidates, {true, true, false, false});

  confirm(candidates.at(0), {});
  checkFeasibility(candidates, {false, false, false, false});
}

TEST_F(PortFinderPlanTests, twoBusesUniqueSlaveId) {
  auto candidates = addBuses(
      {
          {
              {"port 1", "port 2", "port 3"},
              {{"device 1", 1, {{1, 1}}, {}}},
          },
          {
              {"port 1", "port 2", "port 3"},
              {{"device 2", 2, {{1, 1}}, {}}},
          },
      },
      {
          {"device 1", "port 1"},
          {"device 1", "port 2"},
          {"device 1", "port 3"},

          {"device 2", "port 1"},
          {"device 2", "port 2"},
          {"device 2", "port 3"},
      });

  confirm(candidates.at(4), {});
  checkFeasibility(candidates, {true, false, true, false, false, false});

  confirm(candidates.at(2), {});
  checkFeasibility(candidates, {false, false, false, false, false, false});
}

TEST_F(PortFinderPlanTests, twoBusesUniqueRange) {
  auto candidates = addBuses(
      {
          {
              {"port 1", "port 2", "port 3"},
              {{"device 1", 1, {{1, 1}}, {}}},
          },
          {
              {"port 1", "port 2", "port 3"},
              {{"device 2", 1, {{2, 2}}, {}}},
          },
      },
      {
          {"device 1", "port 1"},
          {"device 1", "port 2"},
          {"device 1", "port 3"},

          {"device 2", "port 1"},
          {"device 2", "port 2"},
          {"device 2", "port 3"},
      });

  confirm(candidates.at(1), {});
  checkFeasibility(candidates, {false, false, false, true, false, true});

  confirm(candidates.at(3), {});
  checkFeasibility(candidates, {false, false, false, false, false, false});
}

TEST_F(PortFinderPlanTests, twoBusesUniqueType) {
  auto candidates = addBuses(
      {
          {
              {"port 1", "port 2", "port 3"},
              {{"device 1", 1, {{1, 1}}, {}}},
          },
          {
              {"port 1", "port 2", "port 3"},
              {{"device 2", 1, {}, {{1, 1}}}},
          },
      },
      {
          {"device 1", "port 1"},
          {"device 1", "port 2"},
          {"device 1", "port 3"},

          {"device 2", "port 1"},
          {"device 2", "port 2"},
          {"device 2", "port 3"},
      });

  confirm(candidates.at(1), {});
  checkFeasibility(candidates, {false, false, false, true, false, true});

  confirm(candidates.at(3), {});
  checkFeasibility(candidates, {false, false, false, false, false, false});
}

TEST_F(PortFinderPlanTests, twoBusesSubRange) {
  auto candidates_1 = addBuses(
      {
          {
              {"port 1", "port 2", "port 3"},
              {{"device 1", 1, {{1, 1}}, {}}},
          },
          {
              {"port 1", "port 2", "port 3"},
              {{"device 2", 1, {{1, 2}}, {}}},
          },
      },
      {
          {"device 2", "port 1"},
          {"device 2", "port 2"},
          {"device 2", "port 3"},
      });

  auto candidates_2 = confirm(
      candidates_1.at(1), {{"device 1", "port 1"}, {"device 1", "port 3"}});
  checkFeasibility(candidates_1, {false, false, false});

  confirm(candidates_2.at(0), {});
  checkFeasibility(candidates_1, {false, false, false});
  checkFeasibility(candidates_2, {false, false});
}

TEST_F(PortFinderPlanTests, twoBusesSubType) {
  auto candidates_1 = addBuses(
      {
          {
              {"port 1", "port 2", "port 3"},
              {{"device 1", 1, {{1, 1}}, {}}},
          },
          {
              {"port 1", "port 2", "port 3"},
              {{"device 2", 1, {{1, 1}}, {{1, 1}}}},
          },
      },
      {
          {"device 2", "port 1"},
          {"device 2", "port 2"},
          {"device 2", "port 3"},
      });

  auto candidates_2 = confirm(
      candidates_1.at(1), {{"device 1", "port 1"}, {"device 1", "port 3"}});
  checkFeasibility(candidates_1, {false, false, false});

  confirm(candidates_2.at(0), {});
  checkFeasibility(candidates_1, {false, false, false});
  checkFeasibility(candidates_2, {false, false});
}

TEST_F(PortFinderPlanTests, mutuallyDistinguishableBuses) {
  auto candidates = addBuses(
      {
          {
              {"port 1", "port 2", "port 3"},
              {{"device 1", 1, {{1, 1}}, {}}},
          },
          {
              {"port 1", "port 2", "port 3"},
              {{"device 2", 2, {{1, 1}}, {}}},
          },
          {
              {"port 1", "port 2", "port 3"},
              {{"device 3", 1, {{2, 2}}, {}}},
          },
      },
      {
          {"device 1", "port 1"},
          {"device 1", "port 2"},
          {"device 1", "port 3"},

          {"device 2", "port 1"},
          {"device 2", "port 2"},
          {"device 2", "port 3"},

          {"device 3", "port 1"},
          {"device 3", "port 2"},
          {"device 3", "port 3"},
      });

  confirm(candidates.at(4), {});
  checkFeasibility(candidates, //
      {true, false, true, false, false, false, true, false, true});

  confirm(candidates.at(2), {});
  checkFeasibility(candidates,
      {false, false, false, false, false, false, true, false, false});

  confirm(candidates.at(6), {});
  checkFeasibility(candidates,
      {false, false, false, false, false, false, false, false, false});
}

TEST_F(PortFinderPlanTests, successivelySpecializedBuses) {
  auto candidates_1 = addBuses(
      {
          {
              {"port 1", "port 2", "port 3"},
              {{"device 1", 1, {{1, 1}}, {}}},
          },
          {
              {"port 1", "port 2", "port 3"},
              {{"device 2", 1, {{1, 2}}, {}}},
          },
          {
              {"port 1", "port 2", "port 3"},
              {{"device 3", 1, {{1, 2}}, {}}, {"device 4", 2, {{1, 1}}, {}}},
          },
      },
      {
          {"device 3", "port 1"},
          {"device 4", "port 2"},
          {"device 4", "port 3"},
      });

  auto candidates_2 = confirm(
      candidates_1.at(1), {{"device 2", "port 1"}, {"device 2", "port 3"}});
  checkFeasibility(candidates_1, {false, false, false});

  auto candidates_3 = confirm(candidates_2.at(1), {{"device 1", "port 1"}});
  checkFeasibility(candidates_1, {false, false, false});
  checkFeasibility(candidates_2, {false, false});

  confirm(candidates_3.at(0), {});
  checkFeasibility(candidates_1, {false, false, false});
  checkFeasibility(candidates_2, {false, false});
  checkFeasibility(candidates_3, {false});
}

TEST_F(PortFinderPlanTests, commonGeneralization) {
  auto candidates_1 = addBuses(
      {
          {
              {"port 1", "port 2", "port 3"},
              {{"base 1", 1, {{1, 1}}, {}}},
          },
          {
              {"port 1", "port 2", "port 3"},
              {{"base 2", 2, {{1, 1}}, {}}},
          },
          {
              {"port 1", "port 2", "port 3"},
              {
                  {"generalization 1", 1, {{1, 1}}, {}},
                  {"generalization 2", 2, {{1, 1}}, {}},
              },
          },
      },
      {
          {"generalization 1", "port 1"},
          {"generalization 1", "port 2"},
          {"generalization 2", "port 3"},
      });

  auto candidates_2 = confirm(candidates_1.at(1),
      {
          {"base 1", "port 1"},
          {"base 1", "port 3"},

          {"base 2", "port 1"},
          {"base 2", "port 3"},
      });
  checkFeasibility(candidates_1, {false, false, false});

  confirm(candidates_2.at(0), {});
  checkFeasibility(candidates_1, {false, false, false});
  checkFeasibility(candidates_2, {false, false, false, true});

  confirm(candidates_2.at(3), {});
  checkFeasibility(candidates_1, {false, false, false});
  checkFeasibility(candidates_2, {false, false, false, false});
}

/*
  The next tests call `addBuses` more than once.
*/

TEST_F(PortFinderPlanTests, addUnrelated) {
  auto candidates_1 = addBuses(
      {
          {
              {"port 1", "port 2"},
              {{"device 1", 1, {{1, 1}}, {}}},
          },
          {
              {"port 1", "port 2"},
              {{"device 2", 2, {{1, 1}}, {}}},
          },
      },
      {
          {"device 1", "port 1"},
          {"device 1", "port 2"},

          {"device 2", "port 1"},
          {"device 2", "port 2"},
      });

  auto candidates_2 = addBuses(
      {
          {
              {"port 3", "port 4"},
              {{"device 3", 1, {{1, 1}}, {}}},
          },
          {
              {"port 3", "port 4"},
              {{"device 4", 2, {{1, 1}}, {}}},
          },
      },
      {
          {"device 3", "port 3"},
          {"device 3", "port 4"},

          {"device 4", "port 3"},
          {"device 4", "port 4"},
      });

  confirm(candidates_1.at(0), {});
  checkFeasibility(candidates_1, {false, false, false, true});
  checkFeasibility(candidates_2, {true, true, true, true});

  confirm(candidates_2.at(2), {});
  checkFeasibility(candidates_1, {false, false, false, true});
  checkFeasibility(candidates_2, {false, true, false, false});

  auto candidates_3 = addBuses(
      {
          {
              {"port 5", "port 6"},
              {{"device 5", 1, {{1, 1}}, {}}},
          },
          {
              {"port 5", "port 6"},
              {{"device 6", 2, {{1, 1}}, {}}},
          },
      },
      {
          {"device 5", "port 5"},
          {"device 5", "port 6"},

          {"device 6", "port 5"},
          {"device 6", "port 6"},
      });

  confirm(candidates_3.at(1), {});
  checkFeasibility(candidates_1, {false, false, false, true});
  checkFeasibility(candidates_2, {false, true, false, false});
  checkFeasibility(candidates_3, {false, false, true, false});

  confirm(candidates_3.at(2), {});
  checkFeasibility(candidates_1, {false, false, false, true});
  checkFeasibility(candidates_2, {false, true, false, false});
  checkFeasibility(candidates_3, {false, false, false, false});

  confirm(candidates_1.at(3), {});
  checkFeasibility(candidates_1, {false, false, false, false});
  checkFeasibility(candidates_2, {false, true, false, false});
  checkFeasibility(candidates_3, {false, false, false, false});

  confirm(candidates_2.at(1), {});
  checkFeasibility(candidates_1, {false, false, false, false});
  checkFeasibility(candidates_2, {false, false, false, false});
  checkFeasibility(candidates_3, {false, false, false, false});
}

/*
  "Independent" meaning that there is no dependency from one batch to another
  batch. A dependency would be a candidate that emerges only after confirmation
  of another candidate.
*/
TEST_F(PortFinderPlanTests, addIndependent) {
  auto candidates_1 = addBuses(
      {
          {
              {"port 1", "port 2"},
              {{"device 1", 1, {{1, 1}}, {}}},
          },
          {
              {"port 1", "port 2"},
              {{"device 2", 2, {{1, 1}}, {}}},
          },
      },
      {
          {"device 1", "port 1"},
          {"device 1", "port 2"},

          {"device 2", "port 1"},
          {"device 2", "port 2"},
      });

  auto candidates_2 = addBuses(
      {
          {
              {"port 1", "port 2"},
              {{"device 3", 3, {{1, 1}}, {}}},
          },
          {
              {"port 1", "port 2"},
              {{"device 4", 4, {{1, 1}}, {}}},
          },
      },
      {
          {"device 3", "port 1"},
          {"device 3", "port 2"},

          {"device 4", "port 1"},
          {"device 4", "port 2"},
      });
  checkFeasibility(candidates_1, {true, true, true, true});

  confirm(candidates_1.at(0), {});
  checkFeasibility(candidates_1, {false, false, false, true});
  checkFeasibility(candidates_2, {false, true, false, true});

  auto candidates_3 = addBuses(
      {
          {
              {"port 1", "port 2"},
              {{"device 5", 5, {{1, 1}}, {}}},
          },
          {
              {"port 1", "port 2"},
              {{"device 6", 6, {{1, 1}}, {}}},
          },
      },
      {
          {"device 5", "port 2"},
          {"device 6", "port 2"},
      });
  checkFeasibility(candidates_1, {false, false, false, true});
  checkFeasibility(candidates_2, {false, true, false, true});

  confirm(candidates_3.at(1), {});
  checkFeasibility(candidates_1, {false, false, false, false});
  checkFeasibility(candidates_2, {false, false, false, false});
  checkFeasibility(candidates_3, {false, false});
}

TEST_F(PortFinderPlanTests, addDependors) {
  auto candidates_1 = addBuses(
      {
          {
              {"port 1", "port 2", "port 3"},
              {{"device 1", 1, {{1, 2}}, {}}},
          },
      },
      {{"device 1", "port 1"}, {"device 1", "port 2"}, {"device 1", "port 3"}});

  addBuses(
      {
          {
              {"port 1", "port 2", "port 3"},
              {{"device 2", 1, {{2, 2}}, {}}},
          },
      },
      {});
  checkFeasibility(candidates_1, {true, true, true});

  auto candidates_2 = confirm(
      candidates_1.at(1), {{"device 2", "port 1"}, {"device 2", "port 3"}});
  checkFeasibility(candidates_1, {false, false, false});

  confirm(candidates_2.at(0), {});
  checkFeasibility(candidates_1, {false, false, false});
  checkFeasibility(candidates_2, {false, false});
}

TEST_F(PortFinderPlanTests, addDependees) {
  auto candidates_1 = addBuses(
      {
          {
              {"port 1", "port 2", "port 3"},
              {{"device 1", 1, {{1, 1}}, {}}},
          },
          {
              {"port 1", "port 2", "port 3"},
              {{"device 2", 1, {{2, 2}}, {}}},
          },
      },
      {
          {"device 1", "port 1"},
          {"device 1", "port 2"},
          {"device 1", "port 3"},

          {"device 2", "port 1"},
          {"device 2", "port 2"},
          {"device 2", "port 3"},
      });

  auto candidates_2 = addBuses(
      {
          {
              {"port 1", "port 2", "port 3"},
              {{"device 3", 1, {{1, 2}}, {}}},
          },
      },
      {{"device 3", "port 1"}, {"device 3", "port 2"}, {"device 3", "port 3"}});
  checkFeasibility(candidates_1, {false, false, false, false, false, false});

  auto candidates_3 = confirm(candidates_2.at(1),
      {
          {"device 1", "port 1"},
          {"device 1", "port 3"},

          {"device 2", "port 1"},
          {"device 2", "port 3"},
      });
  checkFeasibility(candidates_1, {true, false, true, true, false, true});
  checkFeasibility(candidates_2, {false, false, false});

  confirm(candidates_3.at(0), {});
  checkFeasibility(candidates_1, {false, false, false, false, false, true});
  checkFeasibility(candidates_2, {false, false, false});
  checkFeasibility(candidates_3, {false, false, false, true});

  confirm(candidates_1.at(5), {});
  checkFeasibility(candidates_1, {false, false, false, false, false, false});
  checkFeasibility(candidates_2, {false, false, false});
  checkFeasibility(candidates_3, {false, false, false, false});
}

/*
  Adds buses which make previously confirmed candidates ambiguous.
*/
TEST_F(PortFinderPlanTests, addDependeesTooLate) {
  auto candidates_1 = addBuses(
      {
          {
              {"port 1", "port 2", "port 3"},
              {{"device 1", 1, {{1, 1}}, {}}},
          },
          {
              {"port 1", "port 2", "port 3"},
              {{"device 2", 1, {{2, 2}}, {}}},
          },
      },
      {
          {"device 1", "port 1"},
          {"device 1", "port 2"},
          {"device 1", "port 3"},

          {"device 2", "port 1"},
          {"device 2", "port 2"},
          {"device 2", "port 3"},
      });

  confirm(candidates_1.at(1), {});
  checkFeasibility(candidates_1, {false, false, false, true, false, true});

  auto candidates_2 = addBuses(
      {
          {
              {"port 1", "port 2", "port 3"},
              {{"device 3", 1, {{1, 2}}, {}}},
          },
      },
      {{"device 3", "port 1"}, {"device 3", "port 3"}});
  checkFeasibility(candidates_1, {false, false, false, false, false, false});

  auto candidates_3 = confirm(candidates_2.at(1), {{"device 2", "port 1"}});
  checkFeasibility(candidates_1, {false, false, false, true, false, false});
  checkFeasibility(candidates_2, {false, false});

  confirm(candidates_3.at(0), {});
  checkFeasibility(candidates_1, {false, false, false, false, false, false});
  checkFeasibility(candidates_2, {false, false});
  checkFeasibility(candidates_3, {false});
}

TEST_F(PortFinderPlanTests, unassign) {
  auto candidates_1 = addBuses(
      {
          {
              {"port 1", "port 2", "port 3"},
              {{"device 1", 1, {{1, 1}}, {}}},
          },
          {
              {"port 1", "port 2", "port 3"},
              {{"device 2", 2, {{1, 1}}, {}}},
          },
          {
              {"port 1", "port 2", "port 3"},
              {{"device 3", 3, {{1, 1}}, {}}},
          },
      },
      {
          {"device 1", "port 1"},
          {"device 1", "port 2"},
          {"device 1", "port 3"},

          {"device 2", "port 1"},
          {"device 2", "port 2"},
          {"device 2", "port 3"},

          {"device 3", "port 1"},
          {"device 3", "port 2"},
          {"device 3", "port 3"},
      });

  confirm(candidates_1.at(0), {});
  checkFeasibility(candidates_1,
      {false, false, false, false, true, true, false, true, true});
  confirm(candidates_1.at(4), {});
  checkFeasibility(candidates_1,
      {false, false, false, false, false, false, false, false, true});
  confirm(candidates_1.at(8), {});
  checkFeasibility(candidates_1,
      {false, false, false, false, false, false, false, false, false});

  // unassign non-last
  auto candidates_2 = unassign("port 1", {{"device 1", "port 1"}});
  checkFeasibility(candidates_1,
      {true, false, false, false, false, false, false, false, false});

  // unassign last
  auto candidates_3 = unassign("port 3",
      {
          {"device 1", "port 3"},
          {"device 3", "port 1"},
          {"device 3", "port 3"},
      });
  checkFeasibility(candidates_1,
      {true, false, true, false, false, false, true, false, true});
  checkFeasibility(candidates_2, {true});
  checkFeasibility(candidates_3, {true, true, true});

  // confirm using new candidate
  confirm(candidates_3.at(0), {});
  checkFeasibility(candidates_1,
      {false, false, false, false, false, false, true, false, false});
  checkFeasibility(candidates_2, {false});
  checkFeasibility(candidates_3, {false, true, false});

  // confirm using old candidate (that was, intermediately, expired)
  confirm(candidates_1.at(6), {});
  checkFeasibility(candidates_1,
      {false, false, false, false, false, false, false, false, false});
  checkFeasibility(candidates_2, {false});
  checkFeasibility(candidates_3, {false, false, false});
}

// NOLINTEND(cert-err58-cpp)
// NOLINTEND(readability-magic-numbers)

} // namespace PortFinderPlanTests
