#ifndef _TECHNOLOGY_ADAPTER_DEMO_READER_HPP
#define _TECHNOLOGY_ADAPTER_DEMO_READER_HPP

#include <Nonempty_Pointer/NonemptyPtr.hpp>
#include <Technology_Adapter_Interface/TechnologyAdapterInterface.hpp>
#include <Threadsafe_Containers/List.hpp>

#include "Readables.hpp"

namespace Technology_Adapter::Demo_Reader {

/**
 * @brief Demonstrates a given Technology Adapter by reading its metrics
 *
 * The Technology Adapter is given via the constructor.
 */
class DemoReader {
  NonemptyPointer::NonemptyPtr<Threadsafe::SharedPtr<Readables>> readables_;
  NonemptyPointer::NonemptyPtr<Threadsafe::SharedPtr<Technology_Adapter::TAI>> const& adapter_;

public:
  DemoReader(NonemptyPointer::NonemptyPtr<Threadsafe::SharedPtr<Technology_Adapter::TAI>> const&);

  void read_all() const; /// @brief Calls all read callbacks and prints results
  void clear(); /// To be called upon stopping

private:
  static void registrate(
      Readables&, Information_Model::NonemptyDevicePtr const&);
  static void registrate(
      Readables&, //
      Information_Model::NonemptyDeviceElementGroupPtr const&,
      ConstString::ConstString const& device_id, //
      size_t indentation);
  static void registrate(
      Technology_Adapter::Demo_Reader::Readables&,
      Information_Model::NonemptyDeviceElementPtr const&,
      ConstString::ConstString const& device_id, //
      size_t indentation);
  static void registrate(
      Technology_Adapter::Demo_Reader::Readables&,
      Information_Model::NonemptyMetricPtr const&,
      ConstString::ConstString const& device_id,
      ConstString::ConstString const& element_id, //
      size_t indentation);
};

} // namespace Technology_Adapter::Demo_Reader

#endif // _MODBUS_TECHNOLOGY_ADAPTER_DEMO_HPP
