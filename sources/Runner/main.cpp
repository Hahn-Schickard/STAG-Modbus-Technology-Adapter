#include <functional>
#include <iostream>

#include "HaSLL/LoggerManager.hpp"
#include "HaSLL/SPD_LoggerRepository.hpp"
#include "Nonempty_Pointer/NonemptyPtr.hpp"

#include "Readables.hpp"
#include "TechnologyAdapterDemoReader.hpp"

#include "./LocalIncludes.hpp"

// NOLINTBEGIN(readability-magic-numbers)

int main(int /*argc*/, char const* /*argv*/[]) {
  try {
    auto logger_repo = std::make_shared<HaSLL::SPD_LoggerRepository>();
    HaSLL::LoggerManager::initialise(logger_repo);

    Technology_Adapter::Demo_Reader::DemoReader reader(
        Technology_Adapter::Demo_Reader::TypeInfo<
            Technology_Adapter::ModbusTechnologyAdapter>(),
        "example_config.json");

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
  } catch (...) {
    std::cerr << "Non-standard exception" << std::endl;
  }
}

// NOLINTEND(readability-magic-numbers)
