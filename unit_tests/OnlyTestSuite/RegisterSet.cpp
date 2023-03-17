#include "RegisterSet.hpp"

#include "gtest/gtest.h"

namespace RegisterSetTests_ {

using Register = Modbus_Technology_Adapter::RegisterIndex;
using SetSpec = std::vector<Modbus_Technology_Adapter::RegisterRange>;
using Result = std::vector<Modbus_Technology_Adapter::RegisterRange>; // sorted
using Instances = std::vector<Register>;

// The range for tests to perform
constexpr Modbus_Technology_Adapter::RegisterIndex min_register = 0;
constexpr Modbus_Technology_Adapter::RegisterIndex max_register = 25;

struct RegisterSetTests : public testing::Test {
  void testConstructor(SetSpec const& ranges, Result const& expected) {
    Modbus_Technology_Adapter::RegisterSet set(ranges);

    Register r = min_register;
    for (auto& interval : expected) {
      for (; r < interval.begin; ++r) {
        EXPECT_FALSE(set.contains(r)) << "r = " << r;
        EXPECT_EQ(set.endOfRange(r), r - 1);
      }
      for (; r <= interval.end; ++r) {
        EXPECT_TRUE(set.contains(r)) << "r = " << r;
        EXPECT_EQ(set.endOfRange(r), interval.end);
      }
    }
    for (; r <= max_register; ++r) {
      EXPECT_FALSE(set.contains(r)) << "r = " << r;
      EXPECT_EQ(set.endOfRange(r), r - 1);
    }
  }
};

TEST_F(RegisterSetTests, empty) { testConstructor(SetSpec(), Result()); }

TEST_F(RegisterSetTests, singleton) {
  testConstructor(SetSpec({{1, 1}}), Result({{1, 1}}));
}

TEST_F(RegisterSetTests, singleRange) {
  testConstructor(SetSpec({{3, 5}}), Result({{3, 5}}));
}

TEST_F(RegisterSetTests, distinctRanges) {
  testConstructor(SetSpec({{3, 5}, {9, 11}, {15, 17}}),
      Result({{3, 5}, {9, 11}, {15, 17}}));
}

TEST_F(RegisterSetTests, contactingRanges) {
  testConstructor(SetSpec({{3, 5}, {6, 8}, {14, 16}, {11, 13}}),
      Result({{3, 8}, {11, 16}}));
}

TEST_F(RegisterSetTests, overlappingRanges) {
  testConstructor(SetSpec({{3, 7}, {5, 9}, {13, 17}, {11, 15}}),
      Result({{3, 9}, {11, 17}}));
}

TEST_F(RegisterSetTests, subRanges) {
  testConstructor(SetSpec({{3, 9}, {5, 7}, {13, 15}, {11, 17}}),
      Result({{3, 9}, {11, 17}}));
}

TEST_F(RegisterSetTests, contactingSubRanges) {
  testConstructor(
      SetSpec({{3, 6}, {3, 7}, {3, 5}, {14, 17}, {13, 17}, {15, 17}}),
      Result({{3, 7}, {13, 17}}));
}

TEST_F(RegisterSetTests, outOfOrder) {
  testConstructor(SetSpec({{9, 11}, {21, 23}, {3, 5}, {15, 17}}),
      Result({{3, 5}, {9, 11}, {15, 17}, {21, 23}}));
}

} // namespace RegisterSetTests_
