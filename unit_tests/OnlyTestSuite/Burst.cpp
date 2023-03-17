#include "Burst.hpp"

#include "gtest/gtest.h"

namespace BurstTests_ {

using TaskSpec = std::vector<int>;
using ReadableSpec = std::vector<Modbus_Technology_Adapter::RegisterRange>;
using BurstsSpec = std::vector<std::pair<int, int>>;
using TaskToPlanSpec = std::vector<size_t>;

struct BurstTests : public testing::Test {
  void testConstructor( //
      TaskSpec const& task, //
      ReadableSpec const& readable, //
      size_t max_burst_size, //
      BurstsSpec const& expected_bursts, //
      TaskToPlanSpec expected_task_to_plan) {

    Modbus_Technology_Adapter::BurstPlan plan(
        task, Modbus_Technology_Adapter::RegisterSet(readable), max_burst_size);

    BurstsSpec actual_bursts;
    for (auto& burst : plan.bursts)
      actual_bursts.push_back(
          std::make_pair(burst.start_register, burst.num_registers));
    EXPECT_EQ(actual_bursts, expected_bursts);

    size_t expected_num_plan_registers = 0;
    for (auto& burst : plan.bursts)
      expected_num_plan_registers += burst.num_registers;
    EXPECT_EQ(plan.num_plan_registers, expected_num_plan_registers);

    EXPECT_EQ(plan.task_to_plan, expected_task_to_plan);
  }
};

TEST_F(BurstTests, noRegisters) {
  testConstructor(TaskSpec(), ReadableSpec(), 1, //
      BurstsSpec(), TaskToPlanSpec());
}

TEST_F(BurstTests, singleRegister) {
  testConstructor(TaskSpec({3}), ReadableSpec({{3, 3}}), 1,
      BurstsSpec({{3, 1}}), TaskToPlanSpec({0}));
}

TEST_F(BurstTests, twoVeryCloseRegistersAscending) {
  testConstructor(TaskSpec({3, 7}), ReadableSpec({{3, 7}}), 100,
      BurstsSpec({{3, 5}}), TaskToPlanSpec({0, 4}));
}

TEST_F(BurstTests, twoVeryCloseRegistersDescending) {
  testConstructor(TaskSpec({7, 3}), ReadableSpec({{3, 7}}), 100,
      BurstsSpec({{3, 5}}), TaskToPlanSpec({4, 0}));
}

TEST_F(BurstTests, twoVeryCloseRegistersRepeated) {
  testConstructor(TaskSpec({3, 7, 3, 7, 7}), ReadableSpec({{3, 7}}), 100,
      BurstsSpec({{3, 5}}), TaskToPlanSpec({0, 4, 0, 4, 4}));
}

TEST_F(BurstTests, twoCloseRegistersAscending) {
  testConstructor(TaskSpec({3, 7}), ReadableSpec({{3, 7}}), 5,
      BurstsSpec({{3, 5}}), TaskToPlanSpec({0, 4}));
}

TEST_F(BurstTests, twoCloseRegistersDescending) {
  testConstructor(TaskSpec({7, 3}), ReadableSpec({{3, 7}}), 5,
      BurstsSpec({{3, 5}}), TaskToPlanSpec({4, 0}));
}

TEST_F(BurstTests, twoCloseRegistersRepeated) {
  testConstructor(TaskSpec({3, 7, 3, 7, 7}), ReadableSpec({{3, 7}}), 5,
      BurstsSpec({{3, 5}}), TaskToPlanSpec({0, 4, 0, 4, 4}));
}

TEST_F(BurstTests, twoRemoteRegistersAscending) {
  testConstructor(TaskSpec({3, 7}), ReadableSpec({{3, 7}}), 4,
      BurstsSpec({{3, 1}, {7, 1}}), TaskToPlanSpec({0, 1}));
}

TEST_F(BurstTests, twoRemoteRegistersDescending) {
  testConstructor(TaskSpec({7, 3}), ReadableSpec({{3, 7}}), 4,
      BurstsSpec({{3, 1}, {7, 1}}), TaskToPlanSpec({1, 0}));
}

TEST_F(BurstTests, twoRemoteRegistersRepeated) {
  testConstructor(TaskSpec({3, 7, 3, 7, 7}), ReadableSpec({{3, 7}}), 4,
      BurstsSpec({{3, 1}, {7, 1}}), TaskToPlanSpec({0, 1, 0, 1, 1}));
}

TEST_F(BurstTests, manyRegisters) {
  testConstructor( //
      TaskSpec( // primes mod 15
          {2, 3, 5, 7, 11, 13, 2, 4, 8, 14, 1, 7, 11, 13, 2, 8, 14, 1, 7, 11,
              13, 4, 8, 14, 7}),
      ReadableSpec({{0, 14}}), 5, //
      BurstsSpec({{1, 5}, {7, 5}, {13, 2}}),
      TaskToPlanSpec({1, 2, 4, 5, 9, 10, 1, 3, 6, 11, 0, 5, 9, 10, 1, 6, 11, 0,
          5, 9, 10, 3, 6, 11, 5}));
}

TEST_F(BurstTests, withGap) {
  testConstructor(TaskSpec({3, 7}), ReadableSpec({{0, 4}, {6, 10}}), 5,
      BurstsSpec({{3, 1}, {7, 1}}), TaskToPlanSpec({0, 1}));
}

TEST_F(BurstTests, notReallyAGap) {
  testConstructor(TaskSpec({3, 7}), ReadableSpec({{0, 5}, {6, 10}}), 5,
      BurstsSpec({{3, 5}}), TaskToPlanSpec({0, 4}));
}

TEST_F(BurstTests, notReallyAGapReversed) {
  testConstructor(TaskSpec({3, 7}), ReadableSpec({{6, 10}, {0, 5}}), 5,
      BurstsSpec({{3, 5}}), TaskToPlanSpec({0, 4}));
}

} // namespace BurstTests_
