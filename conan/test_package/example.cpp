#include "CoAP/Socket.hpp"
#include "LoggerRepository.hpp"

#include <chrono>
#include <iostream>
#include <memory>

using namespace HaSLL;
using namespace CoAP;
using namespace std;

int main() {
  try {
    LoggerRepository::initialise("loggerConfig.json");
    auto logger =
        LoggerRepository::getInstance().registerLoger("Example_Runner");
    unique_ptr<Socket> socket;
    try {
      logger->log(SeverityLevel::INFO, "Instantiating CoAP Socket!");
      socket = make_unique<Socket>();
      socket->start();
      logger->log(SeverityLevel::INFO, "Started CoAP Socket!");
      this_thread::sleep_for(chrono::seconds(1));
      socket->stop();
    } catch (exception &ex) {
      logger->log(SeverityLevel::ERROR, "Received an exception {}", ex.what());
    }
    logger->log(SeverityLevel::INFO, "CoAP Socket was stopped!");
    exit(EXIT_SUCCESS);
  } catch (const exception &ex) {
    cerr << ex.what();
    exit(EXIT_FAILURE);
  }
}