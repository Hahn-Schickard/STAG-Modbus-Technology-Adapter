#include "Burst.hpp"

#include "gtest/gtest.h"

namespace BurstTests_ {

using bursts_spec = std::vector<std::pair<int,int>>;
using task_to_plan_spec = std::vector<size_t>;

struct BurstTests : public testing::Test {
  void testConstructor( //
      Modbus_Technology_Adapter::BurstPlan::Task const& task,
      size_t max_burst_size,
      bursts_spec const& expected_bursts,
      task_to_plan_spec expected_task_to_plan) {

    Modbus_Technology_Adapter::BurstPlan plan(task, max_burst_size);

    bursts_spec actual_bursts;
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
  testConstructor(std::vector<int>(), 1,
      bursts_spec(),
      task_to_plan_spec());
}

TEST_F(BurstTests, singleRegister) {
  testConstructor(std::vector<int>({ 3 }), 1,
      bursts_spec({ {3,1} }),
      task_to_plan_spec({ 0 }));
}

TEST_F(BurstTests, twoVeryCloseRegistersAscending) {
  testConstructor(std::vector<int>({ 3,7 }), 100,
      bursts_spec({ {3,5} }),
      task_to_plan_spec({ 0,4 }));
}

TEST_F(BurstTests, twoVeryCloseRegistersDescending) {
  testConstructor(std::vector<int>({ 7,3 }), 100,
      bursts_spec({ {3,5} }),
      task_to_plan_spec({ 4,0 }));
}

TEST_F(BurstTests, twoVeryCloseRegistersRepeated) {
  testConstructor(std::vector<int>({ 3,7,3,7,7 }), 100,
      bursts_spec({ {3,5} }),
      task_to_plan_spec({ 0,4,0,4,4 }));
}

TEST_F(BurstTests, twoCloseRegistersAscending) {
  testConstructor(std::vector<int>({ 3,7 }), 5,
      bursts_spec({ {3,5} }),
      task_to_plan_spec({ 0,4 }));
}

TEST_F(BurstTests, twoCloseRegistersDescending) {
  testConstructor(std::vector<int>({ 7,3 }), 5,
      bursts_spec({ {3,5} }),
      task_to_plan_spec({ 4,0 }));
}

TEST_F(BurstTests, twoCloseRegistersRepeated) {
  testConstructor(std::vector<int>({ 3,7,3,7,7 }), 5,
      bursts_spec({ {3,5} }),
      task_to_plan_spec({ 0,4,0,4,4 }));
}

TEST_F(BurstTests, twoRemoteRegistersAscending) {
  testConstructor(std::vector<int>({ 3,7 }), 4,
      bursts_spec({ {3,1}, {7,1} }),
      task_to_plan_spec({ 0,1 }));
}

TEST_F(BurstTests, twoRemoteRegistersDescending) {
  testConstructor(std::vector<int>({ 7,3 }), 4,
      bursts_spec({ {3,1}, {7,1} }),
      task_to_plan_spec({ 1,0 }));
}

TEST_F(BurstTests, twoRemoteRegistersRepeated) {
  testConstructor(std::vector<int>({ 3,7,3,7,7 }), 4,
      bursts_spec({ {3,1}, {7,1} }),
      task_to_plan_spec({ 0,1,0,1,1 }));
}

TEST_F(BurstTests, manyRegisters) {
  testConstructor(
      std::vector<int>( // primes mod 15
          { 2,3,5,7,11,13,2,4,8,14,1,7,11,13,2,8,14,1,7,11,13,4,8,14,7 }),
      5,
      bursts_spec({ {1,5}, {7,5}, {13,2} }),
      task_to_plan_spec(
          { 1,2,4,5,9,10,1,3,6,11,0,5,9,10,1,6,11,0,5,9,10,3,6,11,5 }));
}

} // namespace BurstTests_
