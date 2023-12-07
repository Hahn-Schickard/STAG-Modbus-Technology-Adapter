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

  auto logger_repo = std::make_shared<HaSLL::SPD_LoggerRepository>();
  HaSLL::LoggerManager::initialise(logger_repo);

  auto adapter = NonemptyPointer::NonemptyPtr<Threadsafe::SharedPtr<
      Technology_Adapter::ModbusTechnologyAdapter>>::make(
          "example_config.json");

  Technology_Adapter::Demo_Reader::DemoReader reader(adapter);

  for (size_t start_stop_cycle = 0; start_stop_cycle < 2; ++start_stop_cycle) {

    std::cout << "\nStarting\n" << std::endl;
    adapter->start();

    for (size_t read_cycle = 0; read_cycle < 10; ++read_cycle) {
      reader.read_all();
      std::cout << std::endl;
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "\nStopping\n" << std::endl;
    reader.clear();
    adapter->stop();
  }
}

// NOLINTEND(readability-magic-numbers)
