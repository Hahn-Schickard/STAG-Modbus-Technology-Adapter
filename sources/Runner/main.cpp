#include <functional>
#include <iostream>

#include "HaSLL/LoggerManager.hpp"
#include "HaSLL/SPD_LoggerRepository.hpp"
#include "Information_Model/mocks/DeviceMockBuilder.hpp"
#include "Nonempty_Pointer/NonemptyPtr.hpp"
#include "Technology_Adapter_Interface/mocks/ModelRepositoryInterface_MOCK.hpp"

#include "./LocalIncludes.hpp"

// NOLINTBEGIN(readability-magic-numbers)

struct Readable {
  std::string device_id;
  std::function<void()> read_action;
  // NOLINTNEXTLINE(readability-identifier-naming)
  Readable(std::string device_id_, std::function<void()> read_action_)
      : device_id(std::move(device_id_)), read_action(std::move(read_action_)) {
  }
};

using Readables = Threadsafe::List<Readable>;

void browse( //
    std::string const& device_id, //
    Readables& readables, //
    Information_Model::NonemptyDeviceElementGroupPtr const&, //
    size_t);

static const size_t indentation_per_level = 2;

/*
  Print one element (which is a readable metric) as part of the overall printing
  of the information model.
  Furthermore schedule polling of this element.
*/
void browse( //
    std::string const& device_id, //
    Readables& readables, //
    Information_Model::NonemptyMetricPtr const& element, //
    size_t indentation, //
    std::string const& element_id) {

  std::cout //
      << std::string(indentation, ' ') //
      << "Reads " << toString(element->getDataType()) << std::endl;
  std::cout << std::endl;

  readables.emplace_front(device_id, //
      [element, element_id]() {
        try {
          std::cout //
              << element_id << ": " << toString(element->getMetricValue())
              << std::endl;
        } catch (std::exception const& error) {
          std::cout << error.what() << std::endl;
        }
      });
}

/*
  Print one element as part of the overall printing of the information model.
  Furthermore schedule polling where possible.
*/
void browse( //
    std::string const& device_id, //
    Readables& readables, //
    Information_Model::NonemptyDeviceElementPtr const& element, //
    size_t indentation) {

  std::string element_id = element->getElementId();

  std::cout << std::string(indentation, ' ') //
            << "Element name: " << element->getElementName() << std::endl;
  std::cout << std::string(indentation, ' ') //
            << "Element id: " << element_id << std::endl;
  std::cout << std::string(indentation, ' ') //
            << "Described as: " << element->getElementDescription()
            << std::endl;

  match(
      element->functionality,
      [device_id, &readables, indentation](
          Information_Model::NonemptyDeviceElementGroupPtr const& interface) {
        browse(device_id, readables, interface, indentation);
      },
      [device_id, &readables, indentation, element_id](
          Information_Model::NonemptyMetricPtr const& interface) {
        browse(device_id, readables, interface, indentation, element_id);
      },
      [](Information_Model::NonemptyWritableMetricPtr const&) {
        throw std::runtime_error("We don't have writable metrics");
      },
      [](Information_Model::NonemptyFunctionPtr const&) {
        throw std::runtime_error("We don't have functions");
      });
}

/*
  Print one group as part of the overall printing of the information model.
  Furthermore schedule polling where possible.
*/
void browse( //
    std::string const& device_id, //
    Readables& readables, //
    Information_Model::NonemptyDeviceElementGroupPtr const& elements, //
    size_t indentation) {

  std::cout << std::string(indentation, ' ') //
            << "Group contains elements:" << std::endl;
  for (auto const& element : elements->getSubelements()) {
    browse(device_id, readables, element, indentation + indentation_per_level);
  }
}

/*
  Print one device as part of the overall printing of the information model.
  Furthermore schedule polling where possible.
*/
void browse(
    Readables& readables, Information_Model::NonemptyDevicePtr const& device) {

  std::cout << "Device name: " << device->getElementName() << std::endl;
  std::cout << "Device id: " << device->getElementId() << std::endl;
  std::cout << "Described as: " << device->getElementDescription() << std::endl;
  std::cout << std::endl;
  browse(device->getElementId(), readables, device->getDeviceElementGroup(),
      indentation_per_level);
}

int main(int /*argc*/, char const* /*argv*/[]) {
  Readables readables;

  auto logger_repo = std::make_shared<HaSLL::SPD_LoggerRepository>();
  HaSLL::LoggerManager::initialise(logger_repo);

  auto adapter =
      Threadsafe::SharedPtr<Technology_Adapter::ModbusTechnologyAdapter>::make(
          "example_config.json");
  adapter->setInterfaces( //
      NonemptyPointer::make_shared<
          Information_Model::testing::DeviceMockBuilder>(),
      NonemptyPointer::make_shared<::testing::NiceMock<
          Technology_Adapter::testing::ModelRepositoryMock>>(
          [&readables](Information_Model::NonemptyDevicePtr const& device) {
            std::cout << "Registering new device: " << device->getElementName()
                      << std::endl;
            browse(readables, device);
            return true;
          },
          [&readables](std::string const& device_id) {
            std::cout << "Deregistering device " << device_id << std::endl;
            for (auto i = readables.begin(); i != readables.end(); ++i) {
              if (i->device_id == device_id) {
                readables.erase(i);
              }
            }
            return true;
          }));

  for (size_t start_stop_cycle = 0; start_stop_cycle < 2; ++start_stop_cycle) {

    std::cout << "\nStarting\n" << std::endl;

    adapter->start();

    for (size_t read_cycle = 0; read_cycle < 10; ++read_cycle) {
      for (auto& readable : readables) {
        readable.read_action();
      }
      std::cout << std::endl;
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "\nStopping\n" << std::endl;

    readables.clear();

    adapter->stop();
  }
}

// NOLINTEND(readability-magic-numbers)
