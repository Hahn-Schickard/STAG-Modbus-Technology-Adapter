#ifndef _TECHNOLOGY_ADAPTER_DEMO_READER_READABLES_HPP
#define _TECHNOLOGY_ADAPTER_DEMO_READER_READABLES_HPP

#include <functional>
#include <string>

#include <Const_String/ConstString.hpp>
#include <Information_Model/Metric.hpp>
#include <Threadsafe_Containers/List.hpp>

namespace Technology_Adapter::Demo_Reader {

/**
 * @brief Container for readable metrics of all registered devices
 *
 * Thread-safe
 */
class Readables {

  struct Readable {
    Information_Model::NonemptyMetricPtr metric;
    ConstString::ConstString device_id;
    ConstString::ConstString element_id;

    Readable(Information_Model::NonemptyMetricPtr const&,
        ConstString::ConstString const&, ConstString::ConstString const&);
  };

  Threadsafe::List<Readable> readables_;

public:
  void read_all() const; /// @brief Calls all read callbacks and prints results

  /// To be called upon registration
  void add( //
      Information_Model::NonemptyMetricPtr const&,
      ConstString::ConstString const& device_id,
      ConstString::ConstString const& element_id);

  /// To be called upon deregistration
  void remove(ConstString::ConstString const& device_id);
  void clear(); /// To be called upon stopping
};

} // namespace Technology_Adapter::Demo_Reader

#endif // _TECHNOLOGY_ADAPTER_DEMO_READER_READABLES_HPP
