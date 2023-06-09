#include "Index.hpp"

#include "gtest/gtest.h"

namespace IndexTests {

struct T {
  int relevant;
  int irrelevant; // irrelevant to the order
  T(int relevant_, int irrelevant_)
      : relevant(relevant_), irrelevant(irrelevant_) {}
};

struct Compare {
  constexpr bool operator()(T const& x, T const& y) const {
    return x.relevant < y.relevant;
  };
};

// The range for tests to perform
constexpr int max_relevant = 20;
constexpr int max_irrelevant = 10;

struct IndexTests : public testing::Test {
  using Indexing = Technology_Adapter::Modbus::Indexing<T, Compare>;

  Indexing indexing;

  /*
    @pre `expected_contents` is sorted
    @pre both vectors have the same size
  */
  void checkContains(
      std::vector<Indexing::Index> indices,
      std::vector<int>&& expected_contents) {

    EXPECT_EQ(indices.size(), expected_contents.size());

    for (size_t i = 0; i < indices.size(); ++i) {
      Indexing::Index index = indices.at(i);
      int expected = expected_contents.at(i);
      EXPECT_EQ(indexing.get(index).relevant, expected) << i;
      EXPECT_EQ(indexing.get(std::move(index)).relevant, expected) << i;
    }

    int relevant = 0;
    for (auto const& next_expected_relevant : expected_contents) {
      // check gap below `next_expected_relevant`
      for (; relevant < next_expected_relevant; ++relevant) {
        for (int irrelevant = 0; irrelevant <= max_irrelevant; ++irrelevant) {
          T value({relevant, irrelevant});
          EXPECT_FALSE(indexing.contains(value))
              << relevant << "/" << irrelevant;
          EXPECT_FALSE(indexing.contains(std::move(value)))
              << relevant << "/" << irrelevant;

          value = T({relevant, irrelevant});
          EXPECT_ANY_THROW(indexing.index(value))
              << relevant << "/" << irrelevant;
          EXPECT_ANY_THROW(indexing.index(std::move(value)))
              << relevant << "/" << irrelevant;
        }
      }
      // check `next_expected_relevant`
      for (int irrelevant = 0; irrelevant <= max_irrelevant; ++irrelevant) {
        T value({next_expected_relevant, irrelevant});
        EXPECT_TRUE(indexing.contains(value))
            << next_expected_relevant << "/" << irrelevant;
        EXPECT_TRUE(indexing.contains(std::move(value)))
            << next_expected_relevant << "/" << irrelevant;

        value = T({next_expected_relevant, irrelevant});
        Indexing::Index index1 = indexing.index(value);
        EXPECT_EQ(indexing.get(index1).relevant, next_expected_relevant)
            << irrelevant;
        Indexing::Index index2 = indexing.index(std::move(value));
        EXPECT_EQ(index1, index2);
        EXPECT_EQ(indexing.get(std::move(index2)).relevant,
            next_expected_relevant) << irrelevant;
      }
      // check gap above last expected
      relevant = next_expected_relevant + 1;
    }
    for (; relevant <= max_relevant; ++relevant) {
      for (int irrelevant = 0; irrelevant <= max_irrelevant; ++irrelevant) {
        T value({relevant, irrelevant});
        EXPECT_FALSE(indexing.contains(value)) //
            << relevant << "/" << irrelevant;
        EXPECT_FALSE(indexing.contains(std::move(value))) //
            << relevant << "/" << irrelevant;

        value = T({relevant, irrelevant});
        EXPECT_ANY_THROW(indexing.index(value)) //
            << relevant << "/" << irrelevant;
        EXPECT_ANY_THROW(indexing.index(std::move(value))) //
            << relevant << "/" << irrelevant;
      }
    }
  }
};

TEST_F(IndexTests, empty) {
  checkContains({}, {});
}

TEST_F(IndexTests, add) {
  T x1({1, 2});
  auto i1 = indexing.add(x1);
  checkContains({i1}, {1});

  auto i2 = indexing.add({3, 4});
  checkContains({i1, i2}, {1, 3});
}

TEST_F(IndexTests, addAgainThrows) {
  // initializing
  auto i1 = indexing.add({1, 2});
  auto i2 = indexing.add({3, 4});
  checkContains({i1, i2}, {1, 3});

  T x1({1, 5}); // differs in irrelevant part
  EXPECT_ANY_THROW(indexing.add(x1));
  EXPECT_ANY_THROW(indexing.add({3, 6}));
}

TEST_F(IndexTests, emplace) {
  auto i1 = indexing.emplace(1, 2);
  checkContains({i1}, {1});

  auto i2 = indexing.emplace(3, 4);
  checkContains({i1, i2}, {1, 3});
}

} // namespace IndexTests
