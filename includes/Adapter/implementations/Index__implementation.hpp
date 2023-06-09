
#include <stdexcept>

// ComparePtr

template <class T, class Compare>
bool Technology_Adapter::Modbus::Indexing<T, Compare>::ComparePtr::operator()(
    T const* p1, T const* p2) const {

  return compare(*p1, *p2);
}

// Index

template <class T, class Compare>
Technology_Adapter::Modbus::Indexing<T, Compare>::Index::Index(
    ActualIndex index_) : index(index_) {}

template <class T, class Compare>
bool Technology_Adapter::Modbus::Indexing<T, Compare>::Index::operator==(
    Index const& other) const {

  return index == other.index;
}

// Indexing

template <class T, class Compare>
bool Technology_Adapter::Modbus::Indexing<T, Compare>::contains(
    T const& x) const {

  return index_of_value.find(&x) != index_of_value.end();
}

template <class T, class Compare>
bool Technology_Adapter::Modbus::Indexing<T, Compare>::contains(T&& x) const {
  return index_of_value.find(&x) != index_of_value.end();
}

template <class T, class Compare>
typename Technology_Adapter::Modbus::Indexing<T, Compare>::Index
Technology_Adapter::Modbus::Indexing<T, Compare>::index(T const& x) const {
  auto index = index_of_value.find(&x);
  if (index == index_of_value.end()) {
    throw std::runtime_error("Not indexed");
  } else {
    return Index(index->second);
  }
}

template <class T, class Compare>
typename Technology_Adapter::Modbus::Indexing<T, Compare>::Index
Technology_Adapter::Modbus::Indexing<T, Compare>::index(T&& x) const {
  auto index = index_of_value.find(&x);
  if (index == index_of_value.end()) {
    throw std::runtime_error("Not indexed");
  } else {
    return index->second;
  }
}

template <class T, class Compare>
T const& Technology_Adapter::Modbus::Indexing<T, Compare>::get(
    Index const& i) const {

  return *value_of_index.at(i.index);
}

template <class T, class Compare>
T const& Technology_Adapter::Modbus::Indexing<T, Compare>::get(Index&& i) const
{
  return *value_of_index.at(i.index);
}

template <class T, class Compare>
typename Technology_Adapter::Modbus::Indexing<T, Compare>::Index
Technology_Adapter::Modbus::Indexing<T, Compare>::add(T const& x) {
  return add(std::make_shared<T const>(x));
}

template <class T, class Compare>
typename Technology_Adapter::Modbus::Indexing<T, Compare>::Index
Technology_Adapter::Modbus::Indexing<T, Compare>::add(T&& x) {
  return add(std::make_shared<T const>(std::move(x)));
}

template <class T, class Compare>
template <class... Args>
typename Technology_Adapter::Modbus::Indexing<T, Compare>::Index
Technology_Adapter::Modbus::Indexing<T, Compare>::emplace(Args&&... args) {
  return add(std::make_shared<T const>(std::forward<Args>(args)...));
}

template <class T, class Compare>
typename Technology_Adapter::Modbus::Indexing<T, Compare>::Index
Technology_Adapter::Modbus::Indexing<T, Compare>::add(
    std::shared_ptr<T const>&& ptr) {

  auto iterator_and_inserted = index_of_value.emplace(ptr.get(), next_index);

  if (iterator_and_inserted.second) {
    value_of_index.push_back(std::move(ptr));
    ++next_index;
    return iterator_and_inserted.first->second;
  } else {
    throw std::runtime_error("Already indexed");
  }
}
