#include "HaSLL/LoggerManager.hpp"
#include "Modbus_Technology_Adapter/ModbusTechnologyAdapter.hpp"

#include <exception>
#include <iostream>
#include <memory>

using namespace std;

int main() {
  int status = EXIT_SUCCESS;
  try {
    HaSLL::LoggerManager::initialise(HaSLL::makeDefaultRepository());

    auto adapter = make_shared<Technology_Adapter::ModbusTechnologyAdapter>(
        "config/example_config.json");
    try {
      adapter->start();
      cerr << "Adapter started without interfaces being set" << endl;
      status = EXIT_FAILURE;
    } catch (const exception& ex) {
      cout << "Integration test successful." << endl;
    }
  } catch (const exception& ex) {
    cerr << "An unhandled exception occurred during integration test. "
            "Exception: "
         << ex.what() << endl;
    cerr << "Integration test failed" << endl;
    status = EXIT_FAILURE;
  }

  HaSLL::LoggerManager::terminate();
  exit(status);
}
