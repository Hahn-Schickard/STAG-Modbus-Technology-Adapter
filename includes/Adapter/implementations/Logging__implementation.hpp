#include <iostream>

namespace Technology_Adapter::Modbus::Logging {

template <class... Args>
void log(NonemptyPointer::NonemptyPtr<HaSLI::LoggerPtr> logger,
    HaSLI::SeverityLevel severity, Args&& ... args) noexcept {

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
void trace(NonemptyPointer::NonemptyPtr<HaSLI::LoggerPtr> logger,
    Args&& ... args) noexcept {

  log(logger, HaSLI::SeverityLevel::TRACE, std::forward<Args>(args)...);
}

template <class... Args>
void debug(NonemptyPointer::NonemptyPtr<HaSLI::LoggerPtr> logger,
    Args&& ... args) noexcept {

  log(logger, HaSLI::SeverityLevel::DEBUG, std::forward<Args>(args)...);
}

template <class... Args>
void error(NonemptyPointer::NonemptyPtr<HaSLI::LoggerPtr> logger,
    Args&& ... args) noexcept {

  log(logger, HaSLI::SeverityLevel::ERROR, std::forward<Args>(args)...);
}

} // namespace Technology_Adapter::Modbus::Logging
