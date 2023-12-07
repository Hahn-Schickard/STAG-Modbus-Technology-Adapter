#ifndef _TECHNOLOGY_ADAPTER_DEMO_READER_HPP
#define _TECHNOLOGY_ADAPTER_DEMO_READER_HPP

#include <Nonempty_Pointer/NonemptyPtr.hpp>
#include <Technology_Adapter_Interface/TechnologyAdapterInterface.hpp>
#include <Threadsafe_Containers/List.hpp>

#include "Readables.hpp"

namespace Technology_Adapter::Demo_Reader {

/**
 * @brief Template programming helper
 *
 * This class does not do anything. Its sole purpose is to transport information
 * about the template parameter. It is used as a crutch for the templated
 * constructor of `DemoReader`.
 *
 * The point is that a templated constructor cannot be instantiated (because,
 * syntactically, it is not called). Hence template arguments must be deduced.
 * The present class is there to have something to deduce them from.
 */
template <class T> struct TypeInfo {};

/**
 * @brief Demonstrates a given Technology Adapter by reading its metrics
 *
 * The Technology Adapter is given via the constructor.
 * Registration causes printing of information about the registered device.
 */
class DemoReader {
  NonemptyPointer::NonemptyPtr<Threadsafe::SharedPtr<Readables>> readables_;

  // This must be a pointer for the sake of polymorphism
  NonemptyPointer::NonemptyPtr<Threadsafe::SharedPtr<Technology_Adapter::TAI>>
      adapter_;

public:
  template <class TechnologyAdapterImplementation>
  DemoReader(TypeInfo<TechnologyAdapterImplementation>,
      ConstString::ConstString const& config_path);

  void start();
  void read_all() const; /// @brief Calls all read callbacks and prints results
  void stop();

private:
  static void registrate(
      Readables&, Information_Model::NonemptyDevicePtr const&);
  static void registrate( //
      Readables&, //
      Information_Model::NonemptyDeviceElementGroupPtr const&,
      ConstString::ConstString const& device_id, //
      ConstString::ConstString const& indentation_for_printing);
  static void registrate( //
      Technology_Adapter::Demo_Reader::Readables&,
      Information_Model::NonemptyDeviceElementPtr const&,
      ConstString::ConstString const& device_id, //
      ConstString::ConstString const& indentation_for_printing);
  static void registrate( //
      Technology_Adapter::Demo_Reader::Readables&,
      Information_Model::NonemptyMetricPtr const&,
      ConstString::ConstString const& device_id,
      ConstString::ConstString const& element_id, //
      ConstString::ConstString const& indentation_for_printing);
};

} // namespace Technology_Adapter::Demo_Reader

#include "TechnologyAdapterDemoReader_implementation.hpp"

#endif // _MODBUS_TECHNOLOGY_ADAPTER_DEMO_HPP
