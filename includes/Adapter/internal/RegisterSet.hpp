#ifndef _MODBUS_TECHNOLOGY_ADAPTER_REGISTER_SET_HPP
#define _MODBUS_TECHNOLOGY_ADAPTER_REGISTER_SET_HPP

#include <vector>

namespace Technology_Adapter::Modbus {

using RegisterIndex = int; /// following libmodbus

struct RegisterRange {
  RegisterIndex begin;
  RegisterIndex end;

  RegisterRange(RegisterIndex begin_, RegisterIndex end_)
      : begin(begin_), end(end_) {}
};

class RegisterSet {
public:
  class ConstIterator {
  public:
    RegisterIndex operator*() const;
    ConstIterator& operator++();
    bool operator==(ConstIterator const&) const;
    bool operator!=(ConstIterator const&) const;

  private:
    ConstIterator() = delete;
    ConstIterator(std::vector<RegisterRange>::const_iterator,
        std::vector<RegisterRange>::const_iterator, RegisterIndex);

    std::vector<RegisterRange>::const_iterator in_vector_;
    std::vector<RegisterRange>::const_iterator vector_end_;
    RegisterIndex in_range_;

    friend class RegisterSet;
  };

  RegisterSet(std::vector<RegisterRange> const&);
  bool contains(RegisterIndex) const;
  ConstIterator begin() const;
  ConstIterator end() const;

  /**
   * Returns the maximal `r2` such that `r1,...,r2` are members of the set.
   * Returns `r1-1` if `!contains(r1)`.
   */
  RegisterIndex endOfRange(RegisterIndex /*r1*/) const;

  /// @brief subset operator
  bool operator<=(RegisterSet const& other) const;

private:
  std::vector<RegisterRange> const intervals_; // sorted and non-overlapping
};

} // namespace Technology_Adapter::Modbus

#endif // _MODBUS_TECHNOLOGY_ADAPTER_REGISTER_SET_HPP
