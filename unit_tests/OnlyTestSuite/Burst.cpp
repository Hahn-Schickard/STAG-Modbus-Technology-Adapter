#include "Burst.hpp"

#include "gtest/gtest.h"

namespace BurstTests {

using TaskSpec = std::vector<Modbus_Technology_Adapter::RegisterIndex>;
using ReadableSpec = std::vector<Modbus_Technology_Adapter::RegisterRange>;
using BurstsSpec =
    std::vector<std::pair<Modbus_Technology_Adapter::RegisterIndex, int>>;
using TaskToPlanSpec = std::vector<size_t>;

struct BurstPlanTests : public testing::Test {
  static void testConstructor( //
      TaskSpec const& task, //
      ReadableSpec const& readable, //
      size_t max_burst_size, //
      BurstsSpec const& expected_bursts, //
      TaskToPlanSpec const& expected_task_to_plan) {

    Modbus_Technology_Adapter::BurstPlan plan(
        task, Modbus_Technology_Adapter::RegisterSet(readable), max_burst_size);

    BurstsSpec actual_bursts;
    for (auto const& burst : plan.bursts)
      actual_bursts.push_back(
          std::make_pair(burst.start_register, burst.num_registers));
    EXPECT_EQ(actual_bursts, expected_bursts);

    size_t expected_num_plan_registers = 0;
    for (auto const& burst : plan.bursts)
      expected_num_plan_registers += burst.num_registers;
    EXPECT_EQ(plan.num_plan_registers, expected_num_plan_registers);

    EXPECT_EQ(plan.task_to_plan, expected_task_to_plan);
  }
};

// NOLINTBEGIN(cert-err58-cpp)
// NOLINTBEGIN(readability-magic-numbers)

TEST_F(BurstPlanTests, noRegisters) {
  testConstructor(TaskSpec(), ReadableSpec(), 1, //
      BurstsSpec(), TaskToPlanSpec());
}

TEST_F(BurstPlanTests, singleRegister) {
  testConstructor(TaskSpec({3}), ReadableSpec({{3, 3}}), 1,
      BurstsSpec({{3, 1}}), TaskToPlanSpec({0}));
}

TEST_F(BurstPlanTests, twoVeryCloseRegistersAscending) {
  testConstructor(TaskSpec({3, 7}), ReadableSpec({{3, 7}}), 100,
      BurstsSpec({{3, 5}}), TaskToPlanSpec({0, 4}));
}

TEST_F(BurstPlanTests, twoVeryCloseRegistersDescending) {
  testConstructor(TaskSpec({7, 3}), ReadableSpec({{3, 7}}), 100,
      BurstsSpec({{3, 5}}), TaskToPlanSpec({4, 0}));
}

TEST_F(BurstPlanTests, twoVeryCloseRegistersRepeated) {
  testConstructor(TaskSpec({3, 7, 3, 7, 7}), ReadableSpec({{3, 7}}), 100,
      BurstsSpec({{3, 5}}), TaskToPlanSpec({0, 4, 0, 4, 4}));
}

TEST_F(BurstPlanTests, twoCloseRegistersAscending) {
  testConstructor(TaskSpec({3, 7}), ReadableSpec({{3, 7}}), 5,
      BurstsSpec({{3, 5}}), TaskToPlanSpec({0, 4}));
}

TEST_F(BurstPlanTests, twoCloseRegistersDescending) {
  testConstructor(TaskSpec({7, 3}), ReadableSpec({{3, 7}}), 5,
      BurstsSpec({{3, 5}}), TaskToPlanSpec({4, 0}));
}

TEST_F(BurstPlanTests, twoCloseRegistersRepeated) {
  testConstructor(TaskSpec({3, 7, 3, 7, 7}), ReadableSpec({{3, 7}}), 5,
      BurstsSpec({{3, 5}}), TaskToPlanSpec({0, 4, 0, 4, 4}));
}

TEST_F(BurstPlanTests, twoRemoteRegistersAscending) {
  testConstructor(TaskSpec({3, 7}), ReadableSpec({{3, 7}}), 4,
      BurstsSpec({{3, 1}, {7, 1}}), TaskToPlanSpec({0, 1}));
}

TEST_F(BurstPlanTests, twoRemoteRegistersDescending) {
  testConstructor(TaskSpec({7, 3}), ReadableSpec({{3, 7}}), 4,
      BurstsSpec({{3, 1}, {7, 1}}), TaskToPlanSpec({1, 0}));
}

TEST_F(BurstPlanTests, twoRemoteRegistersRepeated) {
  testConstructor(TaskSpec({3, 7, 3, 7, 7}), ReadableSpec({{3, 7}}), 4,
      BurstsSpec({{3, 1}, {7, 1}}), TaskToPlanSpec({0, 1, 0, 1, 1}));
}

TEST_F(BurstPlanTests, withGap) {
  testConstructor(TaskSpec({3, 7}), ReadableSpec({{0, 4}, {6, 10}}), 5,
      BurstsSpec({{3, 1}, {7, 1}}), TaskToPlanSpec({0, 1}));
}

TEST_F(BurstPlanTests, notReallyAGap) {
  testConstructor(TaskSpec({3, 7}), ReadableSpec({{0, 5}, {6, 10}}), 5,
      BurstsSpec({{3, 5}}), TaskToPlanSpec({0, 4}));
}

TEST_F(BurstPlanTests, notReallyAGapReversed) {
  testConstructor(TaskSpec({3, 7}), ReadableSpec({{6, 10}, {0, 5}}), 5,
      BurstsSpec({{3, 5}}), TaskToPlanSpec({0, 4}));
}

TEST_F(BurstPlanTests, nonGreedyOptimum) {
  testConstructor(TaskSpec({1, 2, 6, 7, 8, 12, 13}), ReadableSpec({{0, 15}}), 6,
      BurstsSpec({{1, 2}, {6, 3}, {12, 2}}),
      TaskToPlanSpec({0, 1, 2, 3, 4, 5, 6}));
}

TEST_F(BurstPlanTests, manyRegisters) {
  testConstructor( //
      TaskSpec( // primes mod 15
          {2, 3, 5, 7, 11, 13, 2, 4, 8, 14, 1, 7, 11, 13, 2, 8, 14, 1, 7, 11,
              13, 4, 8, 14, 7}),
      ReadableSpec({{0, 14}}), 5, //
      BurstsSpec({{1, 5}, {7, 2}, {11, 4}}),
      TaskToPlanSpec( //
          {1, 2, 4, 5, 7, 9, 1, 3, 6, 10, 0, 5, 7, 9, 1, 6, 10, 0, 5, 7, //
              9, 3, 6, 10, 5}));
}

// NOLINTEND(readability-magic-numbers)
// NOLINTEND(cert-err58-cpp)

} // namespace BurstTests
