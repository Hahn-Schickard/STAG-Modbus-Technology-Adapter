#include "ModbusTechnologyAdapter.hpp"

namespace Modbus_Technology_Adapter {

ModbusTechnologyAdapter::ModbusTechnologyAdapter()
    : Technology_Adapter::TechnologyAdapter("Modbus Adapter"),
      bus_("/dev/ttyUSB0", 9600, 'N', 8, 2) {}

void ModbusTechnologyAdapter::interfaceSet() {

  /*
    For starters, we model a fixed device.
  */

  // model the device in `builder_interface_`

  Technology_Adapter::DeviceBuilderPtr device_builder = getDeviceBuilder();
  device_builder->buildDeviceBase(
      "EMeter1", "Test E-Meter", "E-Meter used for testing and development");
  std::string phase1 = device_builder->addDeviceElementGroup(
      "Phase 1", "Sensor values of phase 1");
  std::string voltage1 = device_builder->addReadableMetric(phase1, "U1",
      "Effective voltage of phase 1", Information_Model::DataType::DOUBLE,
      [this]() {
        uint16_t result = 13;
        bus_.setSlave(42);
        int read = bus_.readRegisters(35, 1, &result);
        if (read == 0)
          throw "Read failed";
        return Information_Model::DataVariant((double)result);
      });
  std::string current1 = device_builder->addReadableMetric(phase1, "I1",
      "Effective current of phase 1", Information_Model::DataType::DOUBLE,
      [this]() {
        uint16_t result = 13;
        bus_.setSlave(42);
        int read = bus_.readRegisters(36, 1, &result);
        if (read == 0)
          throw "Read failed";
        return Information_Model::DataVariant(((double)result) * 0.1);
      });

  // register device model with `registry_interface_`
  getModelRegistry()->registerDevice(device_builder->getResult());
}

void ModbusTechnologyAdapter::start() {
  bus_.connect();
  Technology_Adapter::TechnologyAdapter::start();
}

void ModbusTechnologyAdapter::stop() {
  Technology_Adapter::TechnologyAdapter::stop();
  bus_.close();
}

} // namespace Modbus_Technology_Adapter
