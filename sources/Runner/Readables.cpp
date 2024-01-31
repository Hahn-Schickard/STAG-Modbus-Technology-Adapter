#include "Readables.hpp"

#include <iostream>

namespace Technology_Adapter::Demo_Reader {

Readables::Readable::Readable(
    // NOLINTBEGIN(modernize-pass-by-value, readability-identifier-naming)
    Information_Model::NonemptyMetricPtr const& metric_,
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    ConstString::ConstString const& device_id_,
    ConstString::ConstString const& element_id_)
    // NOLINTEND(modernize-pass-by-value, readability-identifier-naming)
    : metric(metric_), device_id(device_id_), element_id(element_id_) {}

void Readables::read_all() const {
  for (auto& readable : readables_) {
    try {
      std::cout //
          << "Reading " << (std::string_view)readable.element_id << " gives "
          << toString(readable.metric->getMetricValue()) << std::endl;
    } catch (std::exception const& error) {
      std::cout //
          << "Reading " << (std::string_view)readable.element_id
          << "throws an exception: " << error.what() << std::endl;
    } catch (...) {
      std::cout //
          << "Reading " << (std::string_view)readable.element_id
          << "throws a non-standard exception" << std::endl;
    }
  }
}

void Readables::add( //
    Information_Model::NonemptyMetricPtr const& metric,
    ConstString::ConstString const& device_id,
    ConstString::ConstString const& element_id) {

  readables_.emplace_front(metric, device_id, element_id);
}

void Readables::remove(ConstString::ConstString const& device_id) {
  for (auto i = readables_.begin(); i != readables_.end(); ++i) {
    if (i->device_id == device_id) {
      readables_.erase(i);
    }
  }
}

void Readables::clear() { readables_.clear(); }

} // namespace Technology_Adapter::Demo_Reader
