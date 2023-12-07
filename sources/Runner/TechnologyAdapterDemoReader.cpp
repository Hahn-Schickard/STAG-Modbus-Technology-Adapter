#include "TechnologyAdapterDemoReader.hpp"

namespace Technology_Adapter::Demo_Reader {

char const* const indentation_per_level = "  ";

void DemoReader::start() { adapter_->start(); }

void DemoReader::read_all() const { readables_->read_all(); }

void DemoReader::stop() {
  readables_->clear();
  adapter_->stop();
}

void DemoReader::registrate(
    Technology_Adapter::Demo_Reader::Readables& readables,
    Information_Model::NonemptyDevicePtr const& device) {

  std::cout << "Registering new device" << std::endl;
  std::cout << "Device name: " << device->getElementName() << std::endl;
  std::cout << "Device id: " << device->getElementId() << std::endl;
  std::cout << "Described as: " << device->getElementDescription() << std::endl;
  registrate(readables, device->getDeviceElementGroup(),
      ConstString::ConstString(device->getElementId()), indentation_per_level);
}

void DemoReader::registrate( //
    Readables& readables, //
    Information_Model::NonemptyDeviceElementGroupPtr const& group,
    ConstString::ConstString const& device_id, //
    ConstString::ConstString const& indentation_for_printing) {

  std::cout //
      << (std::string_view)indentation_for_printing
      << "Group contains the following elements:" << std::endl;
  for (auto const& element : group->getSubelements()) {
    registrate(readables, element, device_id,
        indentation_for_printing + indentation_per_level);
  }
}

void DemoReader::registrate(
    Technology_Adapter::Demo_Reader::Readables& readables,
    Information_Model::NonemptyDeviceElementPtr const& element,
    ConstString::ConstString const& device_id, //
    ConstString::ConstString const& indentation_for_printing) {

  ConstString::ConstString element_id(element->getElementId());

  std::cout //
      << (std::string_view)indentation_for_printing //
      << "Element name: " << element->getElementName() << std::endl;
  std::cout //
      << (std::string_view)indentation_for_printing //
      << "Element id: " << (std::string_view)element_id << std::endl;
  std::cout //
      << (std::string_view)indentation_for_printing //
      << "Described as: " << element->getElementDescription() << std::endl;

  match(
      element->functionality,
      [device_id, &readables, indentation_for_printing](
          Information_Model::NonemptyDeviceElementGroupPtr const& interface) {
        registrate(readables, interface, device_id, indentation_for_printing);
      },
      [device_id, &readables, indentation_for_printing, element_id](
          Information_Model::NonemptyMetricPtr const& interface) {
        registrate(readables, interface, device_id, element_id,
            indentation_for_printing);
      },
      [](Information_Model::NonemptyWritableMetricPtr const&) {
        throw std::runtime_error("We don't support writable metrics");
      },
      [](Information_Model::NonemptyFunctionPtr const&) {
        throw std::runtime_error("We don't support functions");
      });
}

void DemoReader::registrate(
    Technology_Adapter::Demo_Reader::Readables& readables,
    Information_Model::NonemptyMetricPtr const& metric,
    // NOLINTBEGIN(bugprone-easily-swappable-parameters)
    ConstString::ConstString const& device_id,
    ConstString::ConstString const& element_id, //
    ConstString::ConstString const& indentation_for_printing) {
    // NOLINTEND(bugprone-easily-swappable-parameters)

  std::cout //
      << (std::string_view)indentation_for_printing //
      << "Reads " << toString(metric->getDataType()) << std::endl;
  std::cout << std::endl;

  readables.add(metric, device_id, element_id);
}

} // namespace Technology_Adapter::Demo_Reader
