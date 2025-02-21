#include <iostream>

namespace Technology_Adapter::Modbus::Logging {

template <class... Args>
void log(Nonempty::Pointer<HaSLL::LoggerPtr> logger,
    HaSLL::SeverityLevel severity, Args&&... args) noexcept {

  try {
    logger->log(severity, std::forward<Args>(args)...);
  } catch (...) {
    /*
      We have encountered an exception during logging. There is not much we can
      do about it. According to dg1024, this is critical. So we do our best to
      provide some feedback. Then we terminate.
    */
    try {
      // maybe not all logging fails
      logger->critical("An exception occured during logging");
    } catch (...) {
      std::cerr << "An exception occured during logging" << std::endl;
    }
    std::exit(-1);
  }
}

template <class... Args>
void trace(
    Nonempty::Pointer<HaSLL::LoggerPtr> logger, Args&&... args) noexcept {

  log(logger, HaSLL::SeverityLevel::Trace, std::forward<Args>(args)...);
}

template <class... Args>
void debug(
    Nonempty::Pointer<HaSLL::LoggerPtr> logger, Args&&... args) noexcept {

  log(logger, HaSLL::SeverityLevel::Debug, std::forward<Args>(args)...);
}

template <class... Args>
void error(
    Nonempty::Pointer<HaSLL::LoggerPtr> logger, Args&&... args) noexcept {

  log(logger, HaSLL::SeverityLevel::Error, std::forward<Args>(args)...);
}

} // namespace Technology_Adapter::Modbus::Logging
