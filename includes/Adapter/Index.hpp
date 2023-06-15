#ifndef _MODBUS_TECHNOLOGY_ADAPTER_INDEX_HPP
#define _MODBUS_TECHNOLOGY_ADAPTER_INDEX_HPP

#include <functional>
#include <map>
#include <memory>

#include "Nonempty_Pointer/NonemptyPtr.hpp"

namespace Technology_Adapter::Modbus {

/**
 * @brief Enumeration of values
 *
 * Assigns contiguous numbers to values of type `T`.
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
  // We do not want to store values of `T` twice. Hence pointers.
  using Map = std::map<T const*, ActualIndex, ComparePtr>;
  using Vector = std::vector<std::shared_ptr<T const>>;

public:
  // An opaque version of `ActualIndex`
  class Index {
    ActualIndex index_;

    Index() = delete;
    Index(ActualIndex);

  public:
    bool operator==(Index const&) const;

    friend class Indexing;
    template <class Key, class Value, class Compare_> friend class IndexMap;
  };

  Indexing() = default;

  bool contains(T const&) const;
  Index lookup(T const& /*x*/) const; /// @pre `contains(x)`
  T const& get(Index const& /*i*/) const; /// @pre `i` pertains to `*this`

  Index add(T const& /*x*/); /// @pre `!contains(x)`
  Index add(T&& /*x*/); /// @pre `!contains(x)`

  /// @pre `!contains(x)` where `x` is the constructed value
  template <class... Args>
  Index emplace(Args&&...);

  Index index(T const&); /// Behaves either like `lookup` or like `add`
  Index index(T&&); /// Behaves either like `lookup` or like `add`

private:
  Vector value_of_index_;
  Map index_of_value_;
  ActualIndex next_index_ = 0;

  // The common code of `emplace` and both `public add`
  Index add(std::shared_ptr<T const>&&);
};

/**
 * @brief Maps `Indexing<Key, Compare>::Index` to `Value`
 *
 * The map is always total; values not defined by `set` are default-constructed.
 *
 * @pre `Value` has a default constructor
 */
template <class Key, class Value, class Compare = std::less<Key>>
class IndexMap {
  using Index = typename Indexing<Key, Compare>::Index;
  using Vector = std::vector<Value>;
public:
  IndexMap() = default;

  Value const& operator()(Index const&) const noexcept;
  Value& operator()(Index const&) noexcept;

  void set(Index const&, Value const&);
  void set(Index const&, Value&&);

  template <class... Args>
  void emplace(Index const&, Args&&...);

private:
  mutable Vector values_;

  void fill(size_t up_to) const;
};

template <class X, class Y, class CompareX = std::less<X>>
class MemoizedFunction {
  using Map = IndexMap<X, std::optional<Y>, CompareX>;

  MemoizedFunction() = delete;
public:
  using Index = typename Indexing<X, CompareX>::Index;
  using Function = std::function<Y(X const&)>;

  MemoizedFunction(
      NonemptyPointer::NonemptyPtr<std::shared_ptr<Indexing<X, CompareX>>>
          const&,
      Function&&);

  Y const& operator()(Index const&) const;
  Y const& operator()(X const&) const;
  Y const& operator()(X&&) const;
private:
  NonemptyPointer::NonemptyPtr<std::shared_ptr<Indexing<X, CompareX>>>
      indexing_;
  Function f_;
  mutable Map map;
};

template <class X1, class X2, class Y, class CompareX1 = std::less<X1>,
    class CompareX2 = std::less<X2>>
class MemoizedBinaryFunction {
  MemoizedBinaryFunction() = delete;
public:
  using Index1 = typename Indexing<X1, CompareX1>::Index;
  using Index2 = typename Indexing<X2, CompareX2>::Index;
  using Function = std::function<Y(X1 const&, X2 const&)>;

  MemoizedBinaryFunction(
      NonemptyPointer::NonemptyPtr<std::shared_ptr<Indexing<X1, CompareX1>>>
          const&,
      NonemptyPointer::NonemptyPtr<std::shared_ptr<Indexing<X2, CompareX2>>>
          const&,
      Function&&);

  /**
   * `Arg1` can be `Index1 const&`, `X1 const&`, or `X1&&`
   *
   * `Arg2` can be `Index2 const&`, `X2 const&`, or `X2&&`
   */
  template <class Arg1, class Arg2>
  Y const& operator()(Arg1&&, Arg2&&) const;
private:
  Function f_;
  MemoizedFunction<X1, MemoizedFunction<X2, Y, CompareX2>, CompareX1> memoized_;
};

} // namespace Technology_Adapter::Modbus

#include "implementations/Index__implementation.hpp"

#endif // _MODBUS_TECHNOLOGY_ADAPTER_INDEX_HPP
