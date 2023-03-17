#ifndef _MODBUS_TECHNOLOGY_ADAPTER_REGISTER_SET_HPP
#define _MODBUS_TECHNOLOGY_ADAPTER_REGISTER_SET_HPP

#include <vector>

namespace Modbus_Technology_Adapter {

using RegisterIndex = int; /// following libmodbus

struct RegisterRange {
  RegisterIndex begin;
  RegisterIndex end;

  RegisterRange(RegisterIndex begin_, RegisterIndex end_)
      : begin(begin_), end(end_) {}
};

class RegisterSet {
public:
  RegisterSet(std::vector<RegisterRange> const&);
  bool contains(RegisterIndex) const;

  /**
   * Returns the maximal `r2` such that `r1,...,r2` are members of the set.
   * Returns `r1-1` if `!contains(r1)`.
   */
  RegisterIndex endOfRange(RegisterIndex /*r1*/) const;

private:
  std::vector<RegisterRange> intervals; // sorted and non-overlapping
};

} // namespace Modbus_Technology_Adapter

#endif // _MODBUS_TECHNOLOGY_ADAPTER_REGISTER_SET_HPP
