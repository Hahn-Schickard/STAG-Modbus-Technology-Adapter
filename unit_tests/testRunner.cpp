#include "gtest/gtest.h"

#include "HaSLL/LoggerManager.hpp"
#include "HaSLL/SPD_LoggerRepository.hpp"

int main(int argc, char** argv) {
  auto repo = std::make_shared<HaSLL::SPD_LoggerRepository>(
      HaSLL::SPD_Configuration(".", "logfile", "", HaSLL::SeverityLevel::TRACE,
          false));
  HaSLL::LoggerManager::initialise(repo);

  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
