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

  void checkDistinctIndices(std::vector<Indexing::Index> const& indices) {
    size_t size = indices.size();
    for (size_t i = 0; i < size; ++i) {
      for (size_t j = 0; j < size; ++j) {
        EXPECT_EQ(indices.at(i) == indices.at(j), i == j) << i << "," << j;
        EXPECT_EQ(indices.at(i) != indices.at(j), i != j) << i << "," << j;
      }
    }
  }

  /*
    @pre `expected_contents` is sorted
    @pre both vectors have the same size
  */
  void checkContains(
      std::vector<Indexing::Index>&& indices,
      std::vector<int>&& expected_contents) {

    checkDistinctIndices(indices);

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
          EXPECT_ANY_THROW(indexing.lookup(value))
              << relevant << "/" << irrelevant;
          EXPECT_ANY_THROW(indexing.lookup(std::move(value)))
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
        Indexing::Index index1 = indexing.lookup(value);
        EXPECT_EQ(indexing.get(index1).relevant, next_expected_relevant)
            << irrelevant;
        Indexing::Index index2 = indexing.lookup(std::move(value));
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
        EXPECT_ANY_THROW(indexing.lookup(value)) //
            << relevant << "/" << irrelevant;
        EXPECT_ANY_THROW(indexing.lookup(std::move(value))) //
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

TEST_F(IndexTests, index) {
  T x1a(1, 2);
  T x1b(1, 4);
  auto i1 = indexing.index(x1a);
  checkContains({i1}, {1});

  auto i2 = indexing.index({3, 4});
  checkContains({i1, i2}, {1, 3});

  EXPECT_EQ(indexing.index(x1b), i1);
  EXPECT_EQ(indexing.index({3, 6}), i2);
}

struct IndexMapTests : public testing::Test {
  using Indexing = Technology_Adapter::Modbus::Indexing<T, Compare>;
  using Map = Technology_Adapter::Modbus::IndexMap<T, int, Compare>;

  Indexing indexing;
  std::vector<Indexing::Index> indices;
  Map map;

  IndexMapTests() {
    indices.push_back(indexing.add({2, 1}));
    indices.push_back(indexing.add({6, 5}));
    indices.push_back(indexing.add({4, 3}));
    indices.push_back(indexing.add({8, 7}));
  }

  void checkEntryConst(Indexing::Index const& x, int y) const {
    EXPECT_EQ(map(x), y);
    EXPECT_EQ(map(Indexing::Index(x)), y);
  }

  void checkEntry(Indexing::Index const& x, int y) {
    checkEntryConst(x, y);
    EXPECT_EQ(map(x), y);
    EXPECT_EQ(map(Indexing::Index(x)), y);
  }

  void checkMap(int y0, int y1, int y2, int y3) {
    /*
      In a first run, we allow for filling of defaults.
      Each overload of `operator()` gets a turn.
    */
    EXPECT_EQ(((Map const&)map)(indices.at(0)), y0);
    EXPECT_EQ(((Map const&)map)(Indexing::Index(indices.at(1))), y1);
    EXPECT_EQ(map(indices.at(2)), y2);
    EXPECT_EQ(map(Indexing::Index(indices.at(3))), y3);

    // In a second run we use each overload for each key/value pair
    checkEntry(indices.at(0), y0);
    checkEntry(indices.at(1), y1);
    checkEntry(indices.at(2), y2);
    checkEntry(indices.at(3), y3);
  }
};

TEST_F(IndexMapTests, empty) {
  checkMap(0, 0, 0, 0);
}

TEST_F(IndexMapTests, set) {
  // initialize
  Indexing::Index x0 = indices.at(0);
  Indexing::Index x1 = indices.at(1);
  Indexing::Index x2 = indices.at(2);
  Indexing::Index x3 = indices.at(3);
  int y0 = 10;
  int y1 = 11;
  int y2 = 12;
  int y3 = 13;
  map.set(x2, y2);
  map.set(std::move(x0), y0);
  map.set(x3, std::move(y3));
  map.set(std::move(x1), std::move(y1));
  checkMap(10, 11, 12, 13);

  // overwrite
  x0 = indices.at(0);
  x1 = indices.at(1);
  y0 = 20;
  y1 = 21;
  y2 = 22;
  y3 = 23;
  map.set(x2, y2);
  map.set(std::move(x0), y0);
  map.set(x3, std::move(y3));
  map.set(std::move(x1), std::move(y1));
  checkMap(20, 21, 22, 23);
}

TEST_F(IndexMapTests, emplace) {
  // initialize
  Indexing::Index x0 = indices.at(0);
  Indexing::Index x1 = indices.at(1);
  Indexing::Index x2 = indices.at(2);
  Indexing::Index x3 = indices.at(3);
  int y0 = 10;
  int y1 = 11;
  int y2 = 12;
  int y3 = 13;
  map.emplace(x2, y2);
  map.emplace(std::move(x0), y0);
  map.emplace(x3, y3);
  map.emplace(std::move(x1), y1);
  checkMap(10, 11, 12, 13);

  // overwrite
  x0 = indices.at(0);
  x1 = indices.at(1);
  y0 = 20;
  y1 = 21;
  y2 = 22;
  y3 = 23;
  map.emplace(x2, y2);
  map.emplace(std::move(x0), y0);
  map.emplace(x3, y3);
  map.emplace(std::move(x1), y1);
  checkMap(20, 21, 22, 23);
}

struct MemoizedFunctionTests : public testing::Test {
  using Indexing = Technology_Adapter::Modbus::Indexing<T, Compare>;
  using Memoized =
      Technology_Adapter::Modbus::MemoizedFunction<T, int, Compare>;

  size_t f_called = 0;
  NonemptyPointer::NonemptyPtr<std::shared_ptr<Indexing>> indexing;
  Memoized f;

  MemoizedFunctionTests()
       : indexing(std::make_shared<Indexing>()),
       f(indexing, [this](T const& x) {
         ++f_called;
         return 2*x.relevant;
       }) {}
};

TEST_F(MemoizedFunctionTests, byIndex) {
  std::vector<Indexing::Index> indices;
  indices.push_back(indexing->add({2, 1}));
  indices.push_back(indexing->add({6, 5}));
  indices.push_back(indexing->add({4, 3}));
  indices.push_back(indexing->add({8, 7}));

  // read once
  EXPECT_EQ(f(indices.at(0)), 4);
  EXPECT_EQ(f_called, 1);
  EXPECT_EQ(f(indices.at(1)), 12);
  EXPECT_EQ(f_called, 2);
  // read again
  EXPECT_EQ(f(indices.at(0)), 4);
  EXPECT_EQ(f(indices.at(1)), 12);
  EXPECT_EQ(f_called, 2);
  // read others once
  EXPECT_EQ(f(indices.at(2)), 8);
  EXPECT_EQ(f_called, 3);
  EXPECT_EQ(f(indices.at(3)), 16);
  EXPECT_EQ(f_called, 4);
  // read all again
  EXPECT_EQ(f(indices.at(0)), 4);
  EXPECT_EQ(f(indices.at(1)), 12);
  EXPECT_EQ(f(indices.at(2)), 8);
  EXPECT_EQ(f(indices.at(3)), 16);
  EXPECT_EQ(f_called, 4);
}

TEST_F(MemoizedFunctionTests, byValue) {
  T x1(2, 1);
  T x3(4, 3);
  // read once
  EXPECT_EQ(f(x1), 4);
  EXPECT_EQ(f_called, 1);
  EXPECT_EQ(f({6, 5}), 12);
  EXPECT_EQ(f_called, 2);
  // read again
  EXPECT_EQ(f(x1), 4);
  EXPECT_EQ(f({6, 0}), 12);
  EXPECT_EQ(f_called, 2);
  // read others once
  EXPECT_EQ(f(x3), 8);
  EXPECT_EQ(f_called, 3);
  EXPECT_EQ(f({8, 7}), 16);
  EXPECT_EQ(f_called, 4);
  // read all again
  EXPECT_EQ(f(x1), 4);
  EXPECT_EQ(f({6, 0}), 12);
  EXPECT_EQ(f(x3), 8);
  EXPECT_EQ(f({8, 7}), 16);
  EXPECT_EQ(f_called, 4);
}

struct MemoizedBinaryFunctionTests : public testing::Test {
  using Indexing = Technology_Adapter::Modbus::Indexing<T, Compare>;
  using Memoized =
      Technology_Adapter::Modbus::MemoizedBinaryFunction<
          T, T, int, Compare, Compare>;

  size_t f_called = 0;
  NonemptyPointer::NonemptyPtr<std::shared_ptr<Indexing>> indexing;
  Memoized f;

  MemoizedBinaryFunctionTests()
       : indexing(std::make_shared<Indexing>()),
       f(indexing, indexing,
           [this](T const& x1, T const& x2) {
             ++f_called;
             return x1.relevant + 2*x2.relevant;
           }) {}
};

TEST_F(MemoizedBinaryFunctionTests, byIndex) {
  Indexing::Index i1 = indexing->add({2, 1});
  Indexing::Index i2 = indexing->add({4, 3});

  // read once
  EXPECT_EQ(f(i1, i1), 6);
  EXPECT_EQ(f_called, 1);
  EXPECT_EQ(f(i2, i2), 12);
  EXPECT_EQ(f_called, 2);
  // read again
  EXPECT_EQ(f(i1, i1), 6);
  EXPECT_EQ(f(i2, i2), 12);
  EXPECT_EQ(f_called, 2);
  // read others once
  EXPECT_EQ(f(i1, i2), 10);
  EXPECT_EQ(f_called, 3);
  EXPECT_EQ(f(i2, i1), 8);
  EXPECT_EQ(f_called, 4);
  // read all again
  EXPECT_EQ(f(i1, i1), 6);
  EXPECT_EQ(f(i2, i2), 12);
  EXPECT_EQ(f(i1, i2), 10);
  EXPECT_EQ(f(i2, i1), 8);
  EXPECT_EQ(f_called, 4);
}

TEST_F(MemoizedBinaryFunctionTests, byValue) {
  T x1(2, 1);

  // read once
  EXPECT_EQ(f(x1, x1), 6);
  EXPECT_EQ(f_called, 1);
  EXPECT_EQ(f(T(4, 3), T(4, 3)), 12);
  EXPECT_EQ(f_called, 2);
  // read again
  EXPECT_EQ(f(x1, x1), 6);
  EXPECT_EQ(f(T(4, 3), T(4, 3)), 12);
  EXPECT_EQ(f_called, 2);
  // read others once
  EXPECT_EQ(f(x1, T(4, 3)), 10);
  EXPECT_EQ(f_called, 3);
  EXPECT_EQ(f(T(4, 3), x1), 8);
  EXPECT_EQ(f_called, 4);
  // read all again
  EXPECT_EQ(f(x1, x1), 6);
  EXPECT_EQ(f(T(4, 3), T(4, 3)), 12);
  EXPECT_EQ(f(x1, T(4, 3)), 10);
  EXPECT_EQ(f(T(4, 3), x1), 8);
  EXPECT_EQ(f_called, 4);
}

TEST_F(MemoizedBinaryFunctionTests, mixed) {
  T x1(2, 1);
  Indexing::Index i1 = indexing->add(x1);
  Indexing::Index i2 = indexing->add({4, 3});

  // read once
  EXPECT_EQ(f(i1, x1), 6);
  EXPECT_EQ(f_called, 1);
  EXPECT_EQ(f(i2, T(4, 3)), 12);
  EXPECT_EQ(f_called, 2);
  // read again
  EXPECT_EQ(f(x1, i1), 6);
  EXPECT_EQ(f(T(4, 3), i2), 12);
  EXPECT_EQ(f_called, 2);
  // read others once
  EXPECT_EQ(f(i1, T(4, 3)), 10);
  EXPECT_EQ(f_called, 3);
  EXPECT_EQ(f(i2, x1), 8);
  EXPECT_EQ(f_called, 4);
  // read all again
  EXPECT_EQ(f(x1, i1), 6);
  EXPECT_EQ(f(T(4, 3), i2), 12);
  EXPECT_EQ(f(x1, i2), 10);
  EXPECT_EQ(f(T(4, 3), i1), 8);
  EXPECT_EQ(f_called, 4);
}

struct IndexSetTests : public testing::Test {
  using Indexing = Technology_Adapter::Modbus::Indexing<T, Compare>;
  using Set = Technology_Adapter::Modbus::IndexSet<T, Compare>;

  Indexing indexing;
  std::vector<Indexing::Index> indices;
  Set set;

  IndexSetTests() {
    indices.push_back(indexing.add({2, 1}));
    indices.push_back(indexing.add({6, 5}));
    indices.push_back(indexing.add({4, 3}));
    indices.push_back(indexing.add({8, 7}));
  }

  void checkSet(
      bool expected0, bool expected1, bool expected2, bool expected3) {

    EXPECT_EQ(set.contains(indices.at(0)), expected0);
    EXPECT_EQ(set.contains(indices.at(1)), expected1);
    EXPECT_EQ(set.contains(indices.at(2)), expected2);
    EXPECT_EQ(set.contains(indices.at(3)), expected3);

    bool found0 = false;
    bool found1 = false;
    bool found2 = false;
    bool found3 = false;
    size_t expected_size = expected0 + expected1 + expected2 + expected3;
    size_t actual_size = 0;
    for (auto const& index : set) {
      ++actual_size;
      if (index == indices.at(0)) {
        found0 = true;
      }
      if (index == indices.at(1)) {
        found1 = true;
      }
      if (index == indices.at(2)) {
        found2 = true;
      }
      if (index == indices.at(3)) {
        found3 = true;
      }
    }
    EXPECT_EQ(actual_size, expected_size);
    EXPECT_EQ(found0, expected0);
    EXPECT_EQ(found1, expected1);
    EXPECT_EQ(found2, expected2);
    EXPECT_EQ(found3, expected3);
  }
};

TEST_F(IndexSetTests, empty) {
  checkSet(false, false, false, false);
}

TEST_F(IndexSetTests, add) {
  // add some
  set.add(indices.at(1));
  checkSet(false, true, false, false);
  set.add(indices.at(3));
  checkSet(false, true, false, true);

  // add again
  set.add(indices.at(1));
  checkSet(false, true, false, true);
  set.add(indices.at(3));
  checkSet(false, true, false, true);

  // add more
  set.add(indices.at(0));
  checkSet(true, true, false, true);
  set.add(indices.at(2));
  checkSet(true, true, true, true);

  // add all again
  set.add(indices.at(0));
  checkSet(true, true, true, true);
  set.add(indices.at(1));
  checkSet(true, true, true, true);
  set.add(indices.at(2));
  checkSet(true, true, true, true);
  set.add(indices.at(3));
  checkSet(true, true, true, true);
}

TEST_F(IndexSetTests, remove) {
  // remove from empty
  set.remove(indices.at(1));
  checkSet(false, false, false, false);
  set.remove(indices.at(3));
  checkSet(false, false, false, false);

  // add some
  set.add(indices.at(1));
  set.add(indices.at(3));
  checkSet(false, true, false, true);

  // remove newest
  set.remove(indices.at(3));
  checkSet(false, true, false, false);

  // add more
  set.add(indices.at(0));
  set.add(indices.at(2));
  checkSet(true, true, true, false);

  // remove middle
  set.remove(indices.at(0));
  checkSet(false, true, true, false);

  // add missing
  set.add(indices.at(0));
  set.add(indices.at(3));
  checkSet(true, true, true, true);

  // remove oldest
  set.remove(indices.at(1));
  checkSet(true, false, true, true);

  // remove all
  set.remove(indices.at(0));
  checkSet(false, false, true, true);
  set.remove(indices.at(1));
  checkSet(false, false, true, true);
  set.remove(indices.at(2));
  checkSet(false, false, false, true);
  set.remove(indices.at(3));
  checkSet(false, false, false, false);
}

} // namespace IndexTests
