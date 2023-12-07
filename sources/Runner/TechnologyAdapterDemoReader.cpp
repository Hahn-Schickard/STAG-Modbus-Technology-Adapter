#include "TechnologyAdapterDemoReader.hpp"

#include <Information_Model/mocks/DeviceMockBuilder.hpp>
#include <Technology_Adapter_Interface/mocks/ModelRepositoryInterface_MOCK.hpp>

namespace Technology_Adapter::Demo_Reader {

static const size_t indentation_per_level = 2;

DemoReader::DemoReader(
    NonemptyPointer::NonemptyPtr<Threadsafe::SharedPtr<Technology_Adapter::TAI>> const& adapter)
    : readables_(
        NonemptyPointer::NonemptyPtr<Threadsafe::SharedPtr<Readables>>::make()),
      adapter_(adapter) {

  auto readables = readables_;

  adapter_->setInterfaces( //
      NonemptyPointer::make_shared<
          Information_Model::testing::DeviceMockBuilder>(),
      NonemptyPointer::make_shared<::testing::NiceMock<
          Technology_Adapter::testing::ModelRepositoryMock>>(
          [readables](Information_Model::NonemptyDevicePtr const& device) {
            registrate(*readables, device);
            return true;
          },
          [readables](std::string const& device_id) {
            std::cout << "Deregistering device " << device_id << std::endl;
            readables->remove(ConstString::ConstString(device_id));
            return true;
          }));
}

void DemoReader::read_all() const { readables_->read_all(); }
void DemoReader::clear() { readables_->clear(); }

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

void DemoReader::registrate(
    Readables& readables, //
    Information_Model::NonemptyDeviceElementGroupPtr const& group,
    ConstString::ConstString const& device_id, //
    size_t indentation) {

  std::cout << std::string(indentation, ' ')
      << "Group contains the following elements:" << std::endl;
  for (auto const& element : group->getSubelements()) {
    registrate(
        readables, element, device_id, indentation + indentation_per_level);
  }
}

void DemoReader::registrate(
    Technology_Adapter::Demo_Reader::Readables& readables,
    Information_Model::NonemptyDeviceElementPtr const& element,
    ConstString::ConstString const& device_id, //
    size_t indentation) {

  ConstString::ConstString element_id(element->getElementId());

  std::cout << std::string(indentation, ' ') //
            << "Element name: " << element->getElementName() << std::endl;
  std::cout << std::string(indentation, ' ') //
            << "Element id: " << (std::string_view)element_id << std::endl;
  std::cout << std::string(indentation, ' ') //
            << "Described as: " << element->getElementDescription()
            << std::endl;

  match(
      element->functionality,
      [device_id, &readables, indentation](
          Information_Model::NonemptyDeviceElementGroupPtr const& interface) {
        registrate(readables, interface, device_id, indentation);
      },
      [device_id, &readables, indentation, element_id](
          Information_Model::NonemptyMetricPtr const& interface) {
        registrate(readables, interface, device_id, element_id, indentation);
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
    ConstString::ConstString const& device_id,
    ConstString::ConstString const& element_id, //
    size_t indentation) {

  std::cout //
      << std::string(indentation, ' ') //
      << "Reads " << toString(metric->getDataType()) << std::endl;
  std::cout << std::endl;

  readables.add(metric, device_id, element_id);
}


} // namespace Technology_Adapter::Demo_Reader
