#include <functional>
#include <iostream>

#include "HaSLL/LoggerManager.hpp"
#include "HaSLL/SPD_LoggerRepository.hpp"
#include "Information_Model/mocks/DeviceMockBuilder.hpp"
#include "Nonempty_Pointer/NonemptyPtr.hpp"
#include "Technology_Adapter_Interface/mocks/ModelRepositoryInterface_MOCK.hpp"

#include "./LocalIncludes.hpp"

// NOLINTBEGIN(readability-magic-numbers)

using Action = std::function<void()>;

struct Actions {
  std::vector<Action> polls; // Polls to do after `start`
};

using ActionsPtr = Threadsafe::MutexSharedPtr<Actions>;

void browse( //
    Actions& actions, //
    Information_Model::NonemptyDeviceElementGroupPtr const&, //
    size_t);

static const size_t indentation_per_level = 2;

/*
  Print one element (which is a readable metric) as part of the overall printing
  of the information model.
  Furthermore schedule polling of this element.
*/
void browse( //
    Actions& actions, //
    Information_Model::NonemptyMetricPtr const& element, //
    size_t indentation, //
    std::string const& element_id) {

  std::cout //
      << std::string(indentation, ' ') //
      << "Reads " << toString(element->getDataType()) << std::endl;
  std::cout << std::endl;

  actions.polls.emplace_back([element, element_id]() {
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
  Print one element (which is a writable metric) as part of the overall printing
  of the information model.
  Furthermore schedule polling of this element.
*/
void browse( //
    Actions& actions, //
    Information_Model::NonemptyWritableMetricPtr const& element, //
    size_t indentation, //
    std::string const& element_id) {

  std::cout << std::string(indentation, ' ') //
            << "Reads " << toString(element->getDataType()) << std::endl;
  std::cout << std::string(indentation, ' ') //
            << "Writes " << toString(element->getDataType()) << " value type"
            << std::endl;
  std::cout << std::endl;

  actions.polls.emplace_back([element, element_id]() {
    std::cout << element_id << ": " << toString(element->getMetricValue())
              << std::endl;
  });
}

/*
  Print one element as part of the overall printing of the information model.
  Furthermore schedule polling where possible.
*/
void browse( //
    Actions& actions, //
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
      [&actions, indentation](
          Information_Model::NonemptyDeviceElementGroupPtr const& interface) {
        browse(actions, interface, indentation);
      },
      [&actions, indentation, element_id](
          Information_Model::NonemptyMetricPtr const& interface) {
        browse(actions, interface, indentation, element_id);
      },
      [&actions, indentation, element_id](
          Information_Model::NonemptyWritableMetricPtr const& interface) {
        browse(actions, interface, indentation, element_id);
      },
      [&actions, indentation, element_id](
          Information_Model::NonemptyFunctionPtr const&) {
        throw std::runtime_error("We don't have functions");
      });
}

/*
  Print one group as part of the overall printing of the information model.
  Furthermore schedule polling where possible.
*/
void browse( //
    Actions& actions, //
    Information_Model::NonemptyDeviceElementGroupPtr const& elements, //
    size_t indentation) {

  std::cout << std::string(indentation, ' ') //
            << "Group contains elements:" << std::endl;
  for (auto const& element : elements->getSubelements()) {
    browse(actions, element, indentation + indentation_per_level);
  }
}

/*
  Print one device as part of the overall printing of the information model.
  Furthermore schedule polling where possible.
*/
void browse(
    Actions& actions, Information_Model::NonemptyDevicePtr const& device) {
  std::cout << "Device name: " << device->getElementName() << std::endl;
  std::cout << "Device id: " << device->getElementId() << std::endl;
  std::cout << "Described as: " << device->getElementDescription() << std::endl;
  std::cout << std::endl;
  browse(actions, device->getDeviceElementGroup(), indentation_per_level);
}

bool registrationHandler(ActionsPtr const& actions,
    Information_Model::NonemptyDevicePtr const& device) {

  std::cout << "Registering new Device: " << device->getElementName()
            << std::endl;
  browse(*actions, device);
  return true;
}

int main(int /*argc*/, char const* /*argv*/[]) {
  auto actions = ActionsPtr::make();

  auto logger_repo = std::make_shared<HaSLL::SPD_LoggerRepository>();
  HaSLL::LoggerManager::initialise(logger_repo);

  auto adapter =
      Threadsafe::SharedPtr<Technology_Adapter::ModbusTechnologyAdapter>::
          make(Technology_Adapter::Modbus::Config::loadConfig(
              "example_config.json"));
  adapter->setInterfaces( //
      NonemptyPointer::make_shared<
          Information_Model::testing::DeviceMockBuilder>(),
      NonemptyPointer::make_shared<::testing::NiceMock<
          Technology_Adapter::testing::ModelRepositoryMock>>(
          std::bind(&registrationHandler, actions, std::placeholders::_1)));

  adapter->start();

  for (int i = 0; i < 10; ++i) {
    for (auto& poll : actions->polls) {
      poll();
    }
    std::cout << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  adapter->stop();
}

// NOLINTEND(readability-magic-numbers)
