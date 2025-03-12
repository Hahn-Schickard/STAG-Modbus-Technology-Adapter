#include "gtest/gtest.h"

#include "HaSLL/LoggerManager.hpp"

int main(int argc, char** argv) {
  HaSLL::LoggerManager::initialise(HaSLL::makeDefaultRepository());

  testing::InitGoogleTest(&argc, argv);
  auto status = RUN_ALL_TESTS();

  HaSLL::LoggerManager::terminate();
  return status;
}
