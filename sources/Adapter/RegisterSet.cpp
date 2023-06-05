#include "RegisterSet.hpp"

#include <map>

namespace Technology_Adapter::Modbus {

using iterator = std::vector<RegisterRange>::const_iterator;

RegisterSet::RegisterSet(std::vector<RegisterRange> const& ranges) {
  if (ranges.empty()) {
    return;
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

  auto i = ends_by_begins.begin();
  // `i` is dereferenceable because `ranges` is non-empty.
  RegisterIndex next_begin = i->first;
  RegisterIndex next_end = i->second;

  while (i != ends_by_begins.end()) {
    if (i->first <= next_end + 1) {
      if (i->second > next_end) {
        next_end = i->second;
      }
    } else {
      intervals.emplace_back(next_begin, next_end);
      next_begin = i->first;
      next_end = i->second;
    }
    ++i;
  }
  intervals.emplace_back(next_begin, next_end);
}

bool RegisterSet::contains(RegisterIndex r) const {
  iterator lower = intervals.begin();
  iterator upper = intervals.end();

  while (lower != upper) {
    /*
      Invariants:
      - `lower` is dereferenceable
      - If `r` is in some interval, then in an interval from `[lower,upper)`.
    */

    iterator middle = lower + (upper - lower) / 2;
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

RegisterIndex RegisterSet::endOfRange(RegisterIndex r) const {
  iterator lower = intervals.begin();
  iterator upper = intervals.end();

  while (lower != upper) {
    /*
      Invariants:
      - `lower` is dereferenceable
      - If `r` is in some interval, then in an interval from `[lower,upper)`.
    */

    iterator middle = lower + (upper - lower) / 2;
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

bool RegisterSet::operator <=(RegisterSet const& other) const {
  iterator other_interval = other.intervals.begin();
  for (auto const& interval : intervals) {
    while ((other_interval != other.intervals.end())
        && (other_interval->end < interval.end)) {

      ++other_interval;
    }
    if ((other_interval == other.intervals.end())
        || (other_interval->begin > interval.begin)) {

      return false;
    }
  }
  return true;
}

} // namespace Technology_Adapter::Modbus
