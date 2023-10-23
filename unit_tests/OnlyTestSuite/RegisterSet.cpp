#include "internal/RegisterSet.hpp"

#include "gtest/gtest.h"

namespace RegisterSetTests {

using Register = Technology_Adapter::Modbus::RegisterIndex;
using SetSpec = std::vector<Technology_Adapter::Modbus::RegisterRange>;
using Result = std::vector<Technology_Adapter::Modbus::RegisterRange>; // sorted
using Instances = std::vector<Register>;

// The range for tests to perform
const Technology_Adapter::Modbus::RegisterIndex min_register = 0;
const Technology_Adapter::Modbus::RegisterIndex max_register = 25;

struct RegisterSetTests : public testing::Test {
  static void testConstructor(SetSpec const& ranges, Result const& expected) {
    Technology_Adapter::Modbus::RegisterSet set(ranges);

    Register r = min_register;
    for (auto const& interval : expected) {
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

    // iteration:

    std::vector<Register> expected_iteration;
    for (auto const& range : expected) {
      for (Register r = range.begin; r <= range.end; ++r) {
        expected_iteration.push_back(r);
      }
    }
    std::vector<Register> actual_iteration;
    for (auto r : set) {
      actual_iteration.push_back(r);
    }
    EXPECT_EQ(actual_iteration, expected_iteration) << "as iteration";

    // Generic subset tests:

    // reflexivity of `<=`
    EXPECT_TRUE(set <= set);

    // equality according to `<=` with `expected`
    equalSets(ranges, expected);

    // min and max
    EXPECT_TRUE(Technology_Adapter::Modbus::RegisterSet({}) <= set);
    EXPECT_TRUE(set <= //
        Technology_Adapter::Modbus::RegisterSet(
            {{min_register, max_register}}));
  }

  static void equalSets(SetSpec const& spec1, SetSpec const& spec2) {
    Technology_Adapter::Modbus::RegisterSet set1(spec1);
    Technology_Adapter::Modbus::RegisterSet set2(spec2);
    EXPECT_TRUE(set1 <= set2);
    EXPECT_TRUE(set2 <= set1);
  }

  // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
  static void properSubset(SetSpec const& smaller, SetSpec const& larger) {
    Technology_Adapter::Modbus::RegisterSet smaller_set(smaller);
    Technology_Adapter::Modbus::RegisterSet larger_set(larger);
    EXPECT_TRUE(smaller_set <= larger_set);
    EXPECT_FALSE(larger_set <= smaller_set);
  }
};

// NOLINTBEGIN(readability-magic-numbers)

TEST_F(RegisterSetTests, empty) { testConstructor(SetSpec(), Result()); }

TEST_F(RegisterSetTests, constructSingleton) {
  testConstructor(SetSpec({{1, 1}}), Result({{1, 1}}));
}

TEST_F(RegisterSetTests, constructSingleRange) {
  testConstructor(SetSpec({{3, 5}}), Result({{3, 5}}));
}

TEST_F(RegisterSetTests, constructDistinctRanges) {
  testConstructor(SetSpec({{3, 5}, {9, 11}, {15, 17}}),
      Result({{3, 5}, {9, 11}, {15, 17}}));
}

TEST_F(RegisterSetTests, constructContactingRanges) {
  testConstructor(SetSpec({{3, 5}, {6, 8}, {14, 16}, {11, 13}}),
      Result({{3, 8}, {11, 16}}));
}

TEST_F(RegisterSetTests, constructOverlappingRanges) {
  testConstructor(SetSpec({{3, 7}, {5, 9}, {13, 17}, {11, 15}}),
      Result({{3, 9}, {11, 17}}));
}

TEST_F(RegisterSetTests, constructSubRanges) {
  testConstructor(SetSpec({{3, 9}, {5, 7}, {13, 15}, {11, 17}}),
      Result({{3, 9}, {11, 17}}));
}

TEST_F(RegisterSetTests, constructContactingSubRanges) {
  testConstructor(
      SetSpec({{3, 6}, {3, 7}, {3, 5}, {14, 17}, {13, 17}, {15, 17}}),
      Result({{3, 7}, {13, 17}}));
}

TEST_F(RegisterSetTests, constructOutOfOrder) {
  testConstructor(SetSpec({{9, 11}, {21, 23}, {3, 5}, {15, 17}}),
      Result({{3, 5}, {9, 11}, {15, 17}, {21, 23}}));
}

TEST_F(RegisterSetTests, subsetSingleRange) {
  properSubset({{3, 5}}, {{3, 6}});
  properSubset({{3, 5}}, {{2, 5}});
  properSubset({{3, 5}}, {{2, 6}});
}

TEST_F(RegisterSetTests, subsetFewerRanges) {
  // empty subset testing is already part of `testConstructor`

  properSubset({{3, 5}}, {{3, 5}, {8, 10}});
  properSubset({{8, 10}}, {{3, 5}, {8, 10}});

  properSubset({{3, 5}}, {{3, 5}, {8, 10}, {13, 15}});
  properSubset({{8, 10}}, {{3, 5}, {8, 10}, {13, 15}});
  properSubset({{13, 15}}, {{3, 5}, {8, 10}, {13, 15}});
  properSubset({{3, 5}, {8, 10}}, {{3, 5}, {8, 10}, {13, 15}});
  properSubset({{3, 5}, {13, 15}}, {{3, 5}, {8, 10}, {13, 15}});
  properSubset({{8, 10}, {13, 15}}, {{3, 5}, {8, 10}, {13, 15}});
}

TEST_F(RegisterSetTests, subsetMergingSuperrange) {
  // disjoint with gap
  properSubset({{3, 5}, {8, 10}}, {{3, 10}});
  properSubset({{3, 5}, {8, 10}}, {{2, 10}});
  properSubset({{3, 5}, {8, 10}}, {{3, 11}});
  properSubset({{3, 5}, {8, 10}}, {{2, 11}});

  // contacting
  equalSets({{3, 6}, {7, 10}}, {{3, 10}});
  properSubset({{3, 6}, {7, 10}}, {{2, 10}});
  properSubset({{3, 6}, {7, 10}}, {{3, 11}});
  properSubset({{3, 6}, {7, 10}}, {{2, 11}});

  // overlapping
  equalSets({{3, 8}, {5, 10}}, {{3, 10}});
  properSubset({{3, 8}, {5, 10}}, {{2, 10}});
  properSubset({{3, 8}, {5, 10}}, {{3, 11}});
  properSubset({{3, 8}, {5, 10}}, {{2, 11}});

  // included
  equalSets({{3, 10}, {5, 8}}, {{3, 10}});
  properSubset({{3, 10}, {5, 8}}, {{2, 10}});
  properSubset({{3, 10}, {5, 8}}, {{3, 11}});
  properSubset({{3, 10}, {5, 8}}, {{2, 11}});
}

// NOLINTEND(readability-magic-numbers)

} // namespace RegisterSetTests
