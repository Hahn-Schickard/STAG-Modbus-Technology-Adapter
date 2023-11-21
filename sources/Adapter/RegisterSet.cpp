#include "internal/RegisterSet.hpp"

#include <map>

namespace Technology_Adapter::Modbus {

using IntervalIterator = std::vector<RegisterRange>::const_iterator;

// ConstIterator

RegisterIndex RegisterSet::ConstIterator::operator*() const {
  return in_range_;
}

RegisterSet::ConstIterator& RegisterSet::ConstIterator::operator++() {
  ++in_range_;
  if (in_range_ > in_vector_->end) {
    ++in_vector_;
    if (in_vector_ != vector_end_) {
      in_range_ = in_vector_->begin;
    }
  }
  return *this;
}

bool RegisterSet::ConstIterator::operator==(ConstIterator const& other) const {
  return (in_vector_ == other.in_vector_) &&
      ((in_vector_ == vector_end_) || (in_range_ == other.in_range_));
}

bool RegisterSet::ConstIterator::operator!=(ConstIterator const& other) const {
  return (in_vector_ != other.in_vector_) ||
      ((in_vector_ != vector_end_) && (in_range_ != other.in_range_));
}

RegisterSet::ConstIterator::ConstIterator(
    std::vector<RegisterRange>::const_iterator in_vector,
    std::vector<RegisterRange>::const_iterator vector_end,
    RegisterIndex in_range)
    : in_vector_(in_vector), vector_end_(vector_end), in_range_(in_range) {}

// RegisterSet

// helper for `RegisterSet` constructor; sorts and eliminates overlaps
std::vector<RegisterRange> intervalsOfRanges(
    std::vector<RegisterRange> const& ranges) {

  if (ranges.empty()) {
    return {};
  }

  // "sort" `ranges` by `begin`
  std::map<RegisterIndex, RegisterIndex> ends_by_begins;
  for (auto const& range : ranges) {
    auto i = ends_by_begins.try_emplace(range.begin, range.end).first;
    // if another range has the same `begin`, use the larger `end`
    if (i->second < range.end) {
      i->second = range.end;
    }
  }

  std::vector<RegisterRange> intervals; // to be returned

  auto i = ends_by_begins.begin();
  // `i` is dereferenceable because `ranges` is non-empty.
  RegisterIndex next_begin = i->first; // next to be added to `intervals`
  RegisterIndex next_end = i->second; // next to be added to `intervals`

  while (i != ends_by_begins.end()) {
    if (i->first <= next_end + 1) {
      // merge `*i` with next interval
      if (i->second > next_end) {
        next_end = i->second;
      }
    } else {
      // add `*i` and start new interval
      intervals.emplace_back(next_begin, next_end);
      next_begin = i->first;
      next_end = i->second;
    }
    ++i;
  }
  intervals.emplace_back(next_begin, next_end);

  return intervals;
}

RegisterSet::RegisterSet(std::vector<RegisterRange> const& ranges)
    : intervals_(intervalsOfRanges(ranges)) {}

bool RegisterSet::contains(RegisterIndex r) const {
  // NOLINTBEGIN(modernize-use-auto)
  IntervalIterator lower = intervals_.begin();
  IntervalIterator upper = intervals_.end();
  // NOLINTEND(modernize-use-auto)

  // This loop implements a binary search
  while (lower != upper) {
    /*
      Invariants:
      - `lower` is dereferenceable
      - If `r` is in some interval, then in an interval from `[lower,upper)`.
    */

    // NOLINTNEXTLINE(modernize-use-auto)
    IntervalIterator middle = lower + (upper - lower) / 2;
    // `middle` is dereferencable

    if (r < middle->begin) {
      upper = middle;
    } else if (r <= middle->end) {
      return true;
    } else {
      lower = middle + 1;
    }
  }
  return false;
}

RegisterSet::ConstIterator RegisterSet::begin() const {
  auto begin = intervals_.cbegin();
  auto end = intervals_.cend();
  return ConstIterator(begin, end, begin == end ? 0 : begin->begin);
}

RegisterSet::ConstIterator RegisterSet::end() const {
  auto end = intervals_.cend();
  return ConstIterator(end, end, 0);
}

RegisterIndex RegisterSet::endOfRange(RegisterIndex r) const {
  // NOLINTBEGIN(modernize-use-auto)
  IntervalIterator lower = intervals_.begin();
  IntervalIterator upper = intervals_.end();
  // NOLINTEND(modernize-use-auto)

  // This loop implements a binary search
  while (lower != upper) {
    /*
      Invariants:
      - `lower` is dereferenceable
      - If `r` is in some interval, then in an interval from `[lower,upper)`.
    */

    // NOLINTNEXTLINE(modernize-use-auto)
    IntervalIterator middle = lower + (upper - lower) / 2;
    // `middle` is dereferencable

    if (r < middle->begin) {
      upper = middle;
    } else if (r <= middle->end) {
      return middle->end;
    } else {
      lower = middle + 1;
    }
  }
  return r - 1;
}

bool RegisterSet::operator<=(RegisterSet const& other) const {
  // NOLINTNEXTLINE(modernize-use-auto)
  IntervalIterator other_interval = other.intervals_.begin();
  for (auto const& interval : intervals_) {
    /*
      Invariants:
      - all intervals before `interval` are subsets of the union of all
        intervals of `other` up to and including `*other_interval`
      - `other_interval` is minimal with the above property
    */

    while ((other_interval != other.intervals_.end()) &&
        (other_interval->end < interval.end)) {

      ++other_interval;
    }
    // Now `other_interval` is (undereferencable or) minimal such that
    // `other_interval->end >= interval.end`

    if ((other_interval == other.intervals_.end()) ||
        (other_interval->begin > interval.begin)) {

      return false;
    }
  }
  return true;
}

} // namespace Technology_Adapter::Modbus
