#include "Burst.hpp"

#include "gtest/gtest.h"

namespace BurstTests {

using TaskSpec = std::vector<Technology_Adapter::Modbus::RegisterIndex>;
using ReadableSpec = std::vector<Technology_Adapter::Modbus::RegisterRange>;
using OneTypeBurstsSpec =
    std::vector<std::pair<Technology_Adapter::Modbus::RegisterIndex, int>>;
using TwoTypesBurstsSpec = std::vector<std::tuple<
    Technology_Adapter::Modbus::RegisterIndex, int, bool /*first type*/>>;
using TaskToPlanSpec = std::vector<size_t>;

struct BurstPlanTests : public testing::Test {
  static Technology_Adapter::Modbus::BurstPlan call(
      TaskSpec const& task, //
      ReadableSpec const& holding, //
      ReadableSpec const& input, //
      size_t max_burst_size) {

    return Technology_Adapter::Modbus::BurstPlan(task,
        Technology_Adapter::Modbus::RegisterSet(holding),
        Technology_Adapter::Modbus::RegisterSet(input),
        max_burst_size);
  }

private:
  using BurstsSpec = std::vector<std::tuple<
      Technology_Adapter::Modbus::RegisterIndex, int, bool /*holding*/>>;

  static void testConstructor( //
      TaskSpec const& task, //
      ReadableSpec const& holding, //
      ReadableSpec const& input, //
      size_t max_burst_size, //
      BurstsSpec const& expected_bursts, // here, "first type" means "holding"
      TaskToPlanSpec const& expected_task_to_plan) {

    Technology_Adapter::Modbus::BurstPlan plan =
        call(task, holding, input, max_burst_size);

    BurstsSpec actual_bursts;
    for (auto const& burst : plan.bursts) {
      actual_bursts.push_back(
          std::make_tuple(burst.start_register, burst.num_registers,
            burst.type == LibModbus::ReadableRegisterType::HoldingRegister));
    }
    EXPECT_EQ(actual_bursts, expected_bursts);

    size_t expected_num_plan_registers = 0;
    for (auto const& burst : plan.bursts) {
      expected_num_plan_registers += burst.num_registers;
    }
    EXPECT_EQ(plan.num_plan_registers, expected_num_plan_registers);

    EXPECT_EQ(plan.task_to_plan, expected_task_to_plan);
  }

public:
  // calls `testConstructor` twice, with both types in turn
  static void testConstructorOneType(
      TaskSpec task_spec, //
      ReadableSpec const& readable, //
      size_t max_burst_size, //
      OneTypeBurstsSpec const& expected_bursts, //
      TaskToPlanSpec const& expected_task_to_plan) {

    BurstsSpec expected_bursts_holding;
    BurstsSpec expected_bursts_input; 
    for (auto burst_spec : expected_bursts) {
      expected_bursts_holding.push_back(std::make_tuple(
          burst_spec.first, burst_spec.second, true));
      expected_bursts_input.push_back(std::make_tuple(
          burst_spec.first, burst_spec.second, false));
    }

    {
      SCOPED_TRACE("Using holding registers");
      testConstructor(task_spec, readable, {}, max_burst_size,
          expected_bursts_holding, expected_task_to_plan);
    }
    {
      SCOPED_TRACE("Using input registers");
      testConstructor(task_spec, {}, readable, max_burst_size,
          expected_bursts_input, expected_task_to_plan);
    }
  }

  // calls `testConstructor` twice; reverses types the second time
  static void testConstructorTwoTypes(
      TaskSpec task_spec, //
      ReadableSpec const& readable_one_type, //
      ReadableSpec const& readable_other_type, //
      size_t max_burst_size, //
      TwoTypesBurstsSpec const& expected_bursts, //
      TaskToPlanSpec const& expected_task_to_plan) {

    BurstsSpec expected_bursts_normal;
    BurstsSpec expected_bursts_reversed; 
    for (auto burst_spec : expected_bursts) {
      expected_bursts_normal.push_back(std::make_tuple(
          std::get<0>(burst_spec), std::get<1>(burst_spec),
          std::get<2>(burst_spec)));
      expected_bursts_reversed.push_back(std::make_tuple(
          std::get<0>(burst_spec), std::get<1>(burst_spec),
          !std::get<2>(burst_spec)));
    }

    {
      SCOPED_TRACE("Using first set as holding registers");
      testConstructor(task_spec, readable_one_type, readable_other_type,
          max_burst_size, expected_bursts_normal, expected_task_to_plan);
    }
    {
      SCOPED_TRACE("Using second set as holding registers");
      testConstructor(task_spec, readable_other_type, readable_one_type,
          max_burst_size, expected_bursts_reversed, expected_task_to_plan);
    }
  }
};

// NOLINTBEGIN(cert-err58-cpp)
// NOLINTBEGIN(readability-magic-numbers)

TEST_F(BurstPlanTests, noRegisters) {
  testConstructorOneType(TaskSpec(), ReadableSpec(), 1, //
      OneTypeBurstsSpec(), TaskToPlanSpec());
}

TEST_F(BurstPlanTests, singleRegister) {
  testConstructorOneType(TaskSpec({3}), ReadableSpec({{3, 3}}), 1,
      OneTypeBurstsSpec({{3, 1}}), TaskToPlanSpec({0}));
}

TEST_F(BurstPlanTests, twoVeryCloseRegistersAscending) {
  testConstructorOneType(TaskSpec({3, 7}), ReadableSpec({{3, 7}}), 100,
      OneTypeBurstsSpec({{3, 5}}), TaskToPlanSpec({0, 4}));
}

TEST_F(BurstPlanTests, twoVeryCloseRegistersDescending) {
  testConstructorOneType(TaskSpec({7, 3}), ReadableSpec({{3, 7}}), 100,
      OneTypeBurstsSpec({{3, 5}}), TaskToPlanSpec({4, 0}));
}

TEST_F(BurstPlanTests, twoVeryCloseRegistersRepeated) {
  testConstructorOneType(TaskSpec({3, 7, 3, 7, 7}), ReadableSpec({{3, 7}}), 100,
      OneTypeBurstsSpec({{3, 5}}), TaskToPlanSpec({0, 4, 0, 4, 4}));
}

TEST_F(BurstPlanTests, twoCloseRegistersAscending) {
  testConstructorOneType(TaskSpec({3, 7}), ReadableSpec({{3, 7}}), 5,
      OneTypeBurstsSpec({{3, 5}}), TaskToPlanSpec({0, 4}));
}

TEST_F(BurstPlanTests, twoCloseRegistersDescending) {
  testConstructorOneType(TaskSpec({7, 3}), ReadableSpec({{3, 7}}), 5,
      OneTypeBurstsSpec({{3, 5}}), TaskToPlanSpec({4, 0}));
}

TEST_F(BurstPlanTests, twoCloseRegistersRepeated) {
  testConstructorOneType(TaskSpec({3, 7, 3, 7, 7}), ReadableSpec({{3, 7}}), 5,
      OneTypeBurstsSpec({{3, 5}}), TaskToPlanSpec({0, 4, 0, 4, 4}));
}

TEST_F(BurstPlanTests, twoRemoteRegistersAscending) {
  testConstructorOneType(TaskSpec({3, 7}), ReadableSpec({{3, 7}}), 4,
      OneTypeBurstsSpec({{3, 1}, {7, 1}}), TaskToPlanSpec({0, 1}));
}

TEST_F(BurstPlanTests, twoRemoteRegistersDescending) {
  testConstructorOneType(TaskSpec({7, 3}), ReadableSpec({{3, 7}}), 4,
      OneTypeBurstsSpec({{3, 1}, {7, 1}}), TaskToPlanSpec({1, 0}));
}

TEST_F(BurstPlanTests, twoRemoteRegistersRepeated) {
  testConstructorOneType(TaskSpec({3, 7, 3, 7, 7}), ReadableSpec({{3, 7}}), 4,
      OneTypeBurstsSpec({{3, 1}, {7, 1}}), TaskToPlanSpec({0, 1, 0, 1, 1}));
}

TEST_F(BurstPlanTests, withGap) {
  testConstructorOneType(TaskSpec({3, 7}), ReadableSpec({{0, 4}, {6, 10}}), 5,
      OneTypeBurstsSpec({{3, 1}, {7, 1}}), TaskToPlanSpec({0, 1}));
}

TEST_F(BurstPlanTests, notReallyAGap) {
  testConstructorOneType(TaskSpec({3, 7}), ReadableSpec({{0, 5}, {6, 10}}), 5,
      OneTypeBurstsSpec({{3, 5}}), TaskToPlanSpec({0, 4}));
}

TEST_F(BurstPlanTests, notReallyAGapReversed) {
  testConstructorOneType(TaskSpec({3, 7}), ReadableSpec({{6, 10}, {0, 5}}), 5,
      OneTypeBurstsSpec({{3, 5}}), TaskToPlanSpec({0, 4}));
}

TEST_F(BurstPlanTests, nonGreedyOptimum) {
  testConstructorOneType(TaskSpec({1, 2, 6, 7, 8, 12, 13}),
      ReadableSpec({{0, 15}}), 6,
      OneTypeBurstsSpec({{1, 2}, {6, 3}, {12, 2}}),
      TaskToPlanSpec({0, 1, 2, 3, 4, 5, 6}));
}

TEST_F(BurstPlanTests, manyRegistersOneType) {
  testConstructorOneType( //
      TaskSpec( // primes mod 15
          {2, 3, 5, 7, 11, 13, 2, 4, 8, 14, 1, 7, 11, 13, 2, 8, 14, 1, 7, 11,
              13, 4, 8, 14, 7}),
      ReadableSpec({{0, 14}}), 5, //
      OneTypeBurstsSpec({{1, 5}, {7, 2}, {11, 4}}),
      TaskToPlanSpec( //
          {1, 2, 4, 5, 7, 9, 1, 3, 6, 10, 0, 5, 7, 9, 1, 6, 10, 0, 5, 7, //
              9, 3, 6, 10, 5}));
}

TEST_F(BurstPlanTests, bothTypesDisjointRanges) {
  testConstructorTwoTypes( //
      TaskSpec({2, 5, 8}),
      ReadableSpec({{1, 3}, {7, 9}}),
      ReadableSpec({{4, 6}}),
      11,
      TwoTypesBurstsSpec({{2, 1, true}, {5, 1, false}, {8, 1, true}}),
      TaskToPlanSpec({0, 1, 2}));
}

TEST_F(BurstPlanTests, bothTypesOverlappingRangesGreedyOptimum) {
  testConstructorTwoTypes( //
      TaskSpec({2, 4, 8}),
      ReadableSpec({{1, 6}}),
      ReadableSpec({{4, 9}}),
      11,
      TwoTypesBurstsSpec({{2, 3, true}, {8, 1, false}}),
      TaskToPlanSpec({0, 2, 3}));
}

TEST_F(BurstPlanTests, bothTypesOverlappingRangesNonGreedyOptimum) {
  testConstructorTwoTypes( //
      TaskSpec({2, 6, 8}),
      ReadableSpec({{1, 6}}),
      ReadableSpec({{4, 9}}),
      11,
      TwoTypesBurstsSpec({{2, 1, true}, {6, 3, false}}),
      TaskToPlanSpec({0, 1, 3}));
}

TEST_F(BurstPlanTests, missingRegister) {
  EXPECT_ANY_THROW(call(TaskSpec({3}), ReadableSpec(), ReadableSpec(), 100));
  EXPECT_ANY_THROW(call( //
    TaskSpec({6}),
    ReadableSpec({{3, 5}, {7, 9}}), ReadableSpec({{3, 5}, {9, 11}}),
    100));
}

// NOLINTEND(readability-magic-numbers)
// NOLINTEND(cert-err58-cpp)

} // namespace BurstTests
