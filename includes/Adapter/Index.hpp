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
 * `Tag` can be used to ensure non-convertibility of `Index`es. If the same
 * `T` and `Compare` are used for two `Indexing`s which are supposed to be
 * different, this difference can be enforced by using different types for
 * `Tag`. Then, accidentally mixing `Index`es of these `Indexing`s will be
 * caught by the type checker.
 *
 * @pre `Compare` fulfills the C++ named requirements `Compare`
 */
template <class T, class Tag = int, class Compare = std::less<T>>
class Indexing {
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
    bool operator!=(Index const&) const;

    friend class Indexing;
    template <class Key, class Value, class Tag_, class Compare_> friend class IndexMap;
  };

  class Iterator {
  public:
    Index index() const; /// @pre valid and dereferenceable

    T const& operator*() const; /// @pre valid and dereferenceable
    T const* operator->() const; /// @pre valid and dereferenceable
    bool operator==(Iterator const& other) const; /// @pre valid
    bool operator!=(Iterator const& other) const; /// @pre valid

    void operator++(); /// @pre valid

  private:
    ActualIndex index_;
    typename Vector::const_iterator vector_iterator_;
    size_t size_;

    Iterator() = delete;
    Iterator(ActualIndex, typename Vector::const_iterator&&, size_t);

    friend class Indexing;
  };

  Indexing() = default;

  bool contains(T const&) const;
  Index lookup(T const& /*x*/) const; /// @pre `contains(x)`
  T const& get(Index const& /*i*/) const; /// @pre `i` pertains to `*this`
  Iterator begin() const;
  Iterator end() const;

  /**
   * @pre `!contains(x)`
   *
   * invalidates all `Iterator`s
   */
  Index add(T const& /*x*/);

  /**
   * @pre `!contains(x)`
   *
   * invalidates all `Iterator`s
   */
  Index add(T&& /*x*/);

  /**
   * @pre `!contains(x)` where `x` is the constructed value
   *
   * invalidates all `Iterator`s
   */
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
template <class Key, class Value, class Tag = int, class Compare = std::less<Key>>
class IndexMap {
  using Vector = std::vector<Value>;

public:
  using Reference = typename Vector::reference;
  using ConstReference = typename Vector::const_reference;
  using Index = typename Indexing<Key, Tag, Compare>::Index;

  IndexMap() = default;

  ConstReference operator()(Index const&) const noexcept;
  Reference operator()(Index const&) noexcept;

  void set(Index const&, Value const&);
  void set(Index const&, Value&&);

  template <class... Args>
  void emplace(Index const&, Args&&...);

private:
  mutable Vector values_;

  void fill(size_t up_to) const;
};

template <class X, class Y, class TagX = int, class CompareX = std::less<X>>
class MemoizedFunction {
  using Map = IndexMap<X, std::optional<Y>, TagX, CompareX>;

  MemoizedFunction() = delete;
public:
  using Index = typename Indexing<X, TagX, CompareX>::Index;
  using Function = std::function<Y(X const&)>;

  MemoizedFunction(
      NonemptyPointer::NonemptyPtr<std::shared_ptr<Indexing<X, TagX, CompareX>>>
          const&,
      Function&&);

  Y const& operator()(Index const&) const;
private:
  NonemptyPointer::NonemptyPtr<std::shared_ptr<Indexing<X, TagX, CompareX>>>
      indexing_;
  Function f_;
  mutable Map map;
};

template <class X1, class X2, class Y, class TagX1 = int, class TagX2 = int, class CompareX1 = std::less<X1>,
    class CompareX2 = std::less<X2>>
class MemoizedBinaryFunction {
  MemoizedBinaryFunction() = delete;
public:
  using Index1 = typename Indexing<X1, TagX1, CompareX1>::Index;
  using Index2 = typename Indexing<X2, TagX2, CompareX2>::Index;
  using Function = std::function<Y(X1 const&, X2 const&)>;

  MemoizedBinaryFunction(
      NonemptyPointer::NonemptyPtr<std::shared_ptr<Indexing<X1, TagX1, CompareX1>>>
          const&,
      NonemptyPointer::NonemptyPtr<std::shared_ptr<Indexing<X2, TagX2, CompareX2>>>
          const&,
      Function&&);

  Y const& operator()(Index1 const&, Index2 const&) const;
private:
  Function f_;
  MemoizedFunction<X1, MemoizedFunction<X2, Y, TagX2, CompareX2>, TagX1, CompareX1> memoized_;
};

template <class T, class Tag = int, class Compare = std::less<T>>
class IndexSet {
public:
  using Index = typename Indexing<T, Tag, Compare>::Index;

private:
  struct List {
    using Ptr = NonemptyPointer::NonemptyPtr<std::shared_ptr<List>>;

    IndexMap<T, std::optional<Index>, Tag, Compare> prev;
    IndexMap<T, std::optional<Index>, Tag, Compare> next;
    std::optional<Index> first;
  };

public:
  /**
   * @brief Iterator for `IndexSet`
   *
   * The order of iteration is not specified. In general, it will be unrelated
   * to `Compare`. It may even be different for different instances of
   * `IndexSet`, even if these contain the same elements.
   */
  class ConstIterator {
  public:
    Index const& operator*() const;
    bool operator==(ConstIterator const&) const;
    bool operator!=(ConstIterator const&) const;

    void operator++();

  private:
    typename List::Ptr list_;
    std::optional<Index> index_;

    ConstIterator() = delete;
    ConstIterator(typename List::Ptr const&);
    ConstIterator(typename List::Ptr const&, std::optional<Index> const&);

    friend IndexSet;
  };

  IndexSet();

  bool contains(Index const&) const;
  ConstIterator begin() const;
  ConstIterator end() const;

  void add(Index const& /*x*/); /// no-op if `contains(x)`
  void remove(Index const& /*x*/); /// no-op if `!contains(x)`

private:
  IndexMap<T, bool, Tag, Compare> present_;
  typename List::Ptr list_;
};

} // namespace Technology_Adapter::Modbus

#include "implementations/Index__implementation.hpp"

#endif // _MODBUS_TECHNOLOGY_ADAPTER_INDEX_HPP
