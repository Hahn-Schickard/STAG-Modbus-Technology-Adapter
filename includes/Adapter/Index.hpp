#ifndef _MODBUS_TECHNOLOGY_ADAPTER_INDEX_HPP
#define _MODBUS_TECHNOLOGY_ADAPTER_INDEX_HPP

#include <functional>
#include <map>
#include <memory>

namespace Technology_Adapter::Modbus {

/**
 * @brief Enumeration of values
 *
 * Assigns contiguous numbers to value of type `T`.
 * Using the numbers instead of the actual values allows for more efficient
 * maps and tables.
 *
 * @pre `Compare` fulfills the C++ named requirements `Compare`
 */
template <class T, class Compare = std::less<T>> class Indexing {
  struct ComparePtr {
    Compare compare;
    bool operator()(T const*, T const*) const;
  };

  using ActualIndex = size_t;
  using Map = std::map<T const*, ActualIndex, ComparePtr>;
  using Vector = std::vector<std::shared_ptr<T const>>;

public:
  // An opaque version of `ActualIndex`
  class Index {
    ActualIndex index;

    Index(ActualIndex);

  public:
    bool operator==(Index const&) const;

    friend class Indexing;
  };

  Indexing() = default;

  bool contains(T const&) const;
  bool contains(T&&) const;
  Index index(T const& /*x*/) const; /// @pre `contains(x)`
  Index index(T&& /*x*/) const; /// @pre `contains(x)`
  T const& get(Index const& /*i*/) const; /// @pre `i` pertains to `*this`
  T const& get(Index&& /*i*/) const; /// @pre `i` pertains to `*this`

  Index add(T const& /*x*/); /// @pre `!contains(x)`
  Index add(T&& /*x*/); /// @pre `!contains(x)`

  /// @pre `!contains(x)` where `x` is the constructed value
  template <class... Args>
  Index emplace(Args&&...);

private:
  Vector value_of_index;
  Map index_of_value;
  ActualIndex next_index = 0;

  // The common code of `emplace` and both `public add`
  Index add(std::shared_ptr<T const>&&);
};

} // namespace Technology_Adapter::Modbus

#include "implementations/Index__implementation.hpp"

#endif // _MODBUS_TECHNOLOGY_ADAPTER_INDEX_HPP
