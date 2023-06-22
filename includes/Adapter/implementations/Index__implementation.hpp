
#include <stdexcept>

// `ComparePtr`

template <class T, class Tag, class Compare>
bool Technology_Adapter::Modbus::Indexing<T, Tag, Compare>::ComparePtr::operator()(
    T const* p1, T const* p2) const {

  return compare(*p1, *p2);
}

// `Indexing::Index`

template <class T, class Tag, class Compare>
Technology_Adapter::Modbus::Indexing<T, Tag, Compare>::Index::Index(
    ActualIndex index) : index_(index) {}

template <class T, class Tag, class Compare>
bool Technology_Adapter::Modbus::Indexing<T, Tag, Compare>::Index::operator==(
    Index const& other) const {

  return index_ == other.index_;
}

template <class T, class Tag, class Compare>
bool Technology_Adapter::Modbus::Indexing<T, Tag, Compare>::Index::operator!=(
    Index const& other) const {

  return index_ != other.index_;
}

// `Indexing::Iterator`

template <class T, class Tag, class Compare>
typename Technology_Adapter::Modbus::Indexing<T, Tag, Compare>::Index 
Technology_Adapter::Modbus::Indexing<T, Tag, Compare>::Iterator::operator*() const {
  if (index_ < size_) {
    return Index(index_);
  } else {
    throw std::runtime_error("Past the end");
  }
}

template <class T, class Tag, class Compare>
bool Technology_Adapter::Modbus::Indexing<T, Tag, Compare>::Iterator::operator==(Iterator const& other) const {
  return index_ == other.index_;
}

template <class T, class Tag, class Compare>
bool Technology_Adapter::Modbus::Indexing<T, Tag, Compare>::Iterator::operator!=(Iterator const& other) const {
  return index_ != other.index_;
}

template <class T, class Tag, class Compare>
void Technology_Adapter::Modbus::Indexing<T, Tag, Compare>::Iterator::operator++() {
  ++index_;
}

template <class T, class Tag, class Compare>
Technology_Adapter::Modbus::Indexing<T, Tag, Compare>::Iterator::Iterator(
    ActualIndex index, size_t size) : index_(index), size_(size) {}

// `Indexing`

template <class T, class Tag, class Compare>
bool Technology_Adapter::Modbus::Indexing<T, Tag, Compare>::contains(
    T const& x) const {

  return index_of_value_.find(&x) != index_of_value_.end();
}

template <class T, class Tag, class Compare>
typename Technology_Adapter::Modbus::Indexing<T, Tag, Compare>::Index
Technology_Adapter::Modbus::Indexing<T, Tag, Compare>::lookup(T const& x) const {
  auto index = index_of_value_.find(&x);
  if (index == index_of_value_.end()) {
    throw std::runtime_error("Not indexed");
  } else {
    return Index(index->second);
  }
}

template <class T, class Tag, class Compare>
T const& Technology_Adapter::Modbus::Indexing<T, Tag, Compare>::get(
    Index const& i) const {

  return *value_of_index_.at(i.index_);
}

template <class T, class Tag, class Compare>
typename Technology_Adapter::Modbus::Indexing<T, Tag, Compare>::Iterator
Technology_Adapter::Modbus::Indexing<T, Tag, Compare>::begin() const {
  return Iterator(0, value_of_index_.size());
}

template <class T, class Tag, class Compare>
typename Technology_Adapter::Modbus::Indexing<T, Tag, Compare>::Iterator
Technology_Adapter::Modbus::Indexing<T, Tag, Compare>::end() const {
  size_t size = value_of_index_.size();
  return Iterator(size, size);
}

template <class T, class Tag, class Compare>
typename Technology_Adapter::Modbus::Indexing<T, Tag, Compare>::Index
Technology_Adapter::Modbus::Indexing<T, Tag, Compare>::add(T const& x) {
  return add(std::make_shared<T const>(x));
}

template <class T, class Tag, class Compare>
typename Technology_Adapter::Modbus::Indexing<T, Tag, Compare>::Index
Technology_Adapter::Modbus::Indexing<T, Tag, Compare>::add(T&& x) {
  return add(std::make_shared<T const>(std::move(x)));
}

template <class T, class Tag, class Compare>
template <class... Args>
typename Technology_Adapter::Modbus::Indexing<T, Tag, Compare>::Index
Technology_Adapter::Modbus::Indexing<T, Tag, Compare>::emplace(Args&&... args) {
  return add(std::make_shared<T const>(std::forward<Args>(args)...));
}

template <class T, class Tag, class Compare>
typename Technology_Adapter::Modbus::Indexing<T, Tag, Compare>::Index
Technology_Adapter::Modbus::Indexing<T, Tag, Compare>::add(
    std::shared_ptr<T const>&& ptr) {

  auto iterator_and_inserted = index_of_value_.emplace(ptr.get(), next_index_);

  if (iterator_and_inserted.second) {
    value_of_index_.push_back(std::move(ptr));
    ++next_index_;
    return iterator_and_inserted.first->second;
  } else {
    throw std::runtime_error("Already indexed");
  }
}

template <class T, class Tag, class Compare>
typename Technology_Adapter::Modbus::Indexing<T, Tag, Compare>::Index
Technology_Adapter::Modbus::Indexing<T, Tag, Compare>::index(T const& x) {
  auto index = index_of_value_.find(&x);
  if (index == index_of_value_.end()) {
    auto entry = std::make_shared<T const>(x);
    index = index_of_value_.emplace(entry.get(), next_index_).first;
    value_of_index_.push_back(std::move(entry));
    ++next_index_;
  }
  return Index(index->second);
}

template <class T, class Tag, class Compare>
typename Technology_Adapter::Modbus::Indexing<T, Tag, Compare>::Index
Technology_Adapter::Modbus::Indexing<T, Tag, Compare>::index(T&& x) {
  auto index = index_of_value_.find(&x);
  if (index == index_of_value_.end()) {
    auto entry = std::make_shared<T const>(std::move(x));
    index = index_of_value_.emplace(entry.get(), next_index_).first;
    value_of_index_.push_back(std::move(entry));
    ++next_index_;
  }
  return Index(index->second);
}

// `IndexMap`

template <class Key, class Value, class Tag, class Compare>
typename Technology_Adapter::Modbus::IndexMap<Key, Value, Tag, Compare>::ConstReference
Technology_Adapter::Modbus::IndexMap<Key, Value, Tag, Compare>::operator()(
    Index const& x) const noexcept {

  fill_before(x.index_+1);
  return values_[x.index_];
}

template <class Key, class Value, class Tag, class Compare>
typename Technology_Adapter::Modbus::IndexMap<Key, Value, Tag, Compare>::Reference
Technology_Adapter::Modbus::IndexMap<Key, Value, Tag, Compare>::operator()(
    Index const& x) noexcept {

  fill_before(x.index_+1);
  return values_[x.index_];
}

template <class Key, class Value, class Tag, class Compare>
void Technology_Adapter::Modbus::IndexMap<Key, Value, Tag, Compare>::set(
    Index const& x, Value const& y) {

  if (values_.size() <= x.index_) {
    fill_before(x.index_);
    values_.emplace_back(y);
  } else {
    values_[x.index_] = y;
  }
}

template <class Key, class Value, class Tag, class Compare>
void Technology_Adapter::Modbus::IndexMap<Key, Value, Tag, Compare>::set(
    Index const& x, Value&& y) {

  if (values_.size() <= x.index_) {
    fill_before(x.index_);
    values_.emplace_back(std::move(y));
  } else {
    values_[x.index_] = std::move(y);
  }
}

template <class Key, class Value, class Tag, class Compare>
template <class... Args>
void Technology_Adapter::Modbus::IndexMap<Key, Value, Tag, Compare>::emplace(
  Index const& x, Args&&... args) {

  if (values_.size() <= x.index_) {
    fill_before(x.index_);
    values_.emplace_back(std::forward<Args...>(args...));
  } else {
    values_[x.index_] = Value(std::forward<Args...>(args...));
  }
}

template <class Key, class Value, class Tag, class Compare>
void Technology_Adapter::Modbus::IndexMap<Key, Value, Tag, Compare>::fill_before(
    size_t where) const {

  for (size_t i = values_.size(); i < where; ++i) {
    values_.emplace_back();
  }
}

// `MemoizedFunction`

template <class X, class Y, class TagX, class CompareX>
Technology_Adapter::Modbus::MemoizedFunction<X, Y, TagX, CompareX>::MemoizedFunction(
    NonemptyPointer::NonemptyPtr<std::shared_ptr<Indexing<X, TagX, CompareX>>> const&
        indexing,
    Function&& f)
    : indexing_(indexing), f_(std::move(f)) {}

template <class X, class Y, class TagX, class CompareX>
Y const&
Technology_Adapter::Modbus::MemoizedFunction<X, Y, TagX, CompareX>::operator()(
    Index const& i) const {

  auto& entry = map_(i);
  if (entry.has_value()) {
    return entry.value();
  } else {
    return entry.emplace(f_(indexing_->get(i)));
  }
}

// `MemoizedBinaryFunction`

template <class X1, class X2, class Y, class TagX1, class TagX2, class CompareX1, class CompareX2>
Technology_Adapter::Modbus::MemoizedBinaryFunction<
    X1, X2, Y, TagX1, TagX2, CompareX1, CompareX2>::MemoizedBinaryFunction(
        NonemptyPointer::NonemptyPtr<std::shared_ptr<Indexing<X1, TagX1, CompareX1>>>
            const& indexing1,
        NonemptyPointer::NonemptyPtr<std::shared_ptr<Indexing<X2, TagX2, CompareX2>>>
            const& indexing2,
        Function&& f)
        : f_(f),
            memoized_(indexing1, [this, indexing2](X1 const& x1) {
              Function* f = &f_;
              return MemoizedFunction<X2, Y, TagX2, CompareX2>(indexing2, [f, x1](
                  X2 const& x2) {

                return (*f)(x1, x2);
              });
            }) {}

template <class X1, class X2, class Y, class TagX1, class TagX2, class CompareX1, class CompareX2>
Y const& Technology_Adapter::Modbus::MemoizedBinaryFunction<
    X1, X2, Y, TagX1, TagX2, CompareX1, CompareX2>::operator()(
        Index1 const& index1, Index2 const& index2) const {

  return memoized_(index1)(index2);
}

// `IndexSet::ConstIterator`

template <class T, class Tag, class Compare>
typename Technology_Adapter::Modbus::Indexing<T, Tag, Compare>::Index const&
Technology_Adapter::Modbus::IndexSet<T, Tag, Compare>::ConstIterator::operator*(
    ) const {

  return index_.value();
}

template <class T, class Tag, class Compare>
bool
Technology_Adapter::Modbus::IndexSet<T, Tag, Compare>::ConstIterator::operator==(
    ConstIterator const& other) const {

  return index_ == other.index_;
}

template <class T, class Tag, class Compare>
bool
Technology_Adapter::Modbus::IndexSet<T, Tag, Compare>::ConstIterator::operator!=(
    ConstIterator const& other) const {

  return index_ != other.index_;
}

template <class T, class Tag, class Compare>
void
Technology_Adapter::Modbus::IndexSet<T, Tag, Compare>::ConstIterator::operator++() {
  index_ = list_->next(index_.value());
}

template <class T, class Tag, class Compare>
Technology_Adapter::Modbus::IndexSet<T, Tag, Compare>::ConstIterator::ConstIterator(
        typename List::Ptr const& list)
    : list_(list) {}

template <class T, class Tag, class Compare>
Technology_Adapter::Modbus::IndexSet<T, Tag, Compare>::ConstIterator::ConstIterator(
        typename List::Ptr const& list, std::optional<Index> const& index)
    : list_(list), index_(index) {}

// `IndexSet`

template <class T, class Tag, class Compare>
Technology_Adapter::Modbus::IndexSet<T, Tag, Compare>::IndexSet()
    : list_(std::make_shared<List>()) {}

template <class T, class Tag, class Compare>
bool Technology_Adapter::Modbus::IndexSet<T, Tag, Compare>::contains(
    Index const& index) const {

  return present_(index);
}

template <class T, class Tag, class Compare>
typename Technology_Adapter::Modbus::IndexSet<T, Tag, Compare>::ConstIterator
Technology_Adapter::Modbus::IndexSet<T, Tag, Compare>::begin() const {
  return ConstIterator(list_, list_->first);
}

template <class T, class Tag, class Compare>
typename Technology_Adapter::Modbus::IndexSet<T, Tag, Compare>::ConstIterator
Technology_Adapter::Modbus::IndexSet<T, Tag, Compare>::end() const {
  return ConstIterator(list_);
}

template <class T, class Tag, class Compare>
void Technology_Adapter::Modbus::IndexSet<T, Tag, Compare>::add(Index const& index) {
  if (!present_(index)) {
    present_.set(index, true);

    auto& first = list_->first;
    if (first.has_value()) {
      list_->prev.set(first.value(), index);
    }
    list_->prev.set(index, {});
    list_->next.set(index, first);
    first = index;
  }
}

template <class T, class Tag, class Compare>
void Technology_Adapter::Modbus::IndexSet<T, Tag, Compare>::remove(
    Index const& index) {

  if (present_(index)) {
    present_.set(index, false);

    auto& prev = list_->prev(index);
    auto& next = list_->next(index);
    if (prev.has_value()) {
      list_->next.set(prev.value(), next);
    } else {
      list_->first = next;
    }
    if (next.has_value()) {
      list_->prev.set(next.value(), prev);
    }
  }
}
