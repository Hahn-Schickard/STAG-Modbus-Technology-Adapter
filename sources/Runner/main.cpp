#include <iostream>

#include "HaSLL/LoggerManager.hpp"

#include "ModbusTechnologyAdapter.hpp"

#include "TechnologyAdapterDemoReader.hpp"

// NOLINTBEGIN(readability-magic-numbers)

int main(int /*argc*/, char const* /*argv*/[]) {
  int status = EXIT_SUCCESS;
  try {
    HaSLL::LoggerManager::initialise(HaSLL::makeDefaultRepository());

    Technology_Adapter::Demo_Reader::DemoReader reader(
        Technology_Adapter::Demo_Reader::TypeInfo<
            Technology_Adapter::ModbusTechnologyAdapter>(),
        "config/example_config.json");

    for ( //
        size_t start_stop_cycle = 0; start_stop_cycle < 2; ++start_stop_cycle) {

      std::cout << "\nStarting\n" << std::endl;
      reader.start();

      for (size_t read_cycle = 0; read_cycle < 10; ++read_cycle) {
        reader.read_all();
        std::cout << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
      }

      std::cout << "\nStopping\n" << std::endl;
      reader.stop();
    }
  } catch (std::exception const& error) {
    std::cerr << "Exception: " << error.what() << std::endl;
    status = EXIT_FAILURE;
  } catch (...) {
    std::cerr << "Non-standard exception" << std::endl;
    status = EXIT_FAILURE;
  }
  HaSLL::LoggerManager::terminate();
  exit(status);
}

// NOLINTEND(readability-magic-numbers)
