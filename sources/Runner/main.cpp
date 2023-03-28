#include <functional>
#include <iostream>

#include "HaSLL/LoggerManager.hpp"
#include "HaSLL/SPD_LoggerRepository.hpp"
#include "Information_Model/mocks/DeviceMockBuilder.hpp"
#include "Technology_Adapter_Interface/mocks/ModelRegistryInterface_MOCK.hpp"

#include "Config.hpp"
#include "LibmodbusAbstraction.hpp"
#include "ModbusTechnologyAdapter.hpp"

using Action = std::function<void()>;

struct Actions {
  std::vector<Action> polls; // Polls to do after `start`
};

using ActionsPtr = Threadsafe::MutexSharedPtr<Actions>;

void browse( //
    Actions& actions, //
    Information_Model::NonemptyDeviceElementGroupPtr const&, //
    size_t);

static constexpr size_t indentation_per_level = 2;

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

  std::cout << std::string(indentation, ' ') //
            << "Reads " << toString(element->getDataType()) << std::endl;
  std::cout << std::endl;

  actions.polls.emplace_back([element, element_id]() {
    std::cout << element_id << ": " << toString(element->getMetricValue())
              << std::endl;
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

  match(element->specific_interface,
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
void browse(Actions& actions, Information_Model::DevicePtr const& device) {
  std::cout << "Device name: " << device->getElementName() << std::endl;
  std::cout << "Device id: " << device->getElementId() << std::endl;
  std::cout << "Described as: " << device->getElementDescription() << std::endl;
  std::cout << std::endl;
  browse(actions, device->getDeviceElementGroup(), indentation_per_level);
}

bool registrationHandler(
    ActionsPtr const& actions, Information_Model::DevicePtr const& device) {

  std::cout << "Registering new Device: " << device->getElementName()
            << std::endl;
  browse(*actions, device);
  return true;
}

void config_add_phase( //
    Modbus_Technology_Adapter::Config::Device& device, //
    std::string&& name, std::string&& description, //
    int base_register) {

  auto& group =
      device.subgroups.emplace_back(std::move(name), std::move(description));
  group.readables.emplace_back("U", "Effective voltage",
      Information_Model::DataType::DOUBLE, std::vector<int>{base_register},
      [](std::vector<uint16_t> const& registers)
          -> Information_Model::DataVariant { //
        return (double)registers[0];
      });
  group.readables.emplace_back("I", "Effective current",
      Information_Model::DataType::DOUBLE, std::vector<int>{base_register + 1},
      [](std::vector<uint16_t> const& registers)
          -> Information_Model::DataVariant { //
        return ((double)registers[0]) * 0.1;
      });
}

Modbus_Technology_Adapter::Config::Bus make_config() {
  Modbus_Technology_Adapter::Config::Bus bus(
      "/dev/ttyUSB0", 9600, LibModbus::Parity::None, 8, 2);

  auto& device = bus.devices.emplace_back( //
      "EMeter1", "Test E-Meter", "E-Meter used for testing and development", //
      42, 1, //
      std::vector<Modbus_Technology_Adapter::RegisterRange>({{27, 51}}));

  device.readables.emplace_back("WT1", "Total energy consumption Tariff 1",
      Information_Model::DataType::DOUBLE, std::vector<int>{27, 28},
      [](std::vector<uint16_t> const& registers)
          -> Information_Model::DataVariant {
        return ((double)registers[0]) * 655.36 + ((double)registers[1]) * 0.01;
      });

  config_add_phase(device, "Phase 1", "Sensor values of phase 1", 35);
  config_add_phase(device, "Phase 2", "Sensor values of phase 2", 40);
  config_add_phase(device, "Phase 3", "Sensor values of phase 3", 45);
  return bus;
}

int main(int argc, char const* /*argv*/[]) {

  try {
    auto actions = ActionsPtr::make();

    auto logger_repo = std::make_shared<HaSLL::SPD_LoggerRepository>(
        "config/loggerConfig.json");
    HaSLL::LoggerManager::initialise(logger_repo);

    auto adapter = Threadsafe::
        SharedPtr<Modbus_Technology_Adapter::ModbusTechnologyAdapter>::make(
            make_config());
    adapter->setInterfaces(
        std::make_shared<Information_Model::testing::DeviceMockBuilder>(),
        std::make_shared<::testing::NiceMock<
            Technology_Adapter::testing::ModelRegistryMock>>(
            std::make_shared<Technology_Adapter::testing::RegistrationHandler>(
                std::bind(
                    &registrationHandler, actions, std::placeholders::_1))));

    adapter->start();

    for (int i = 0; i < 10; ++i) {
      for (auto& poll : actions->polls)
        poll();
      std::cout << std::endl;
    }

    adapter->stop();
  } catch (LibModbus::ModbusError const& error) {
    bool ignore_modbus_errors = argc > 1;
    if (ignore_modbus_errors) {
      std::cout << "libmodbus error: " << error.what() << std::endl;
    } else
      throw;
  }
}
