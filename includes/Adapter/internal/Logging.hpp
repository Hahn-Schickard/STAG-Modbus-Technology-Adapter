#ifndef _MODBUS_TECHNOLOGY_ADAPTER_LOGGING_HPP
#define _MODBUS_TECHNOLOGY_ADAPTER_LOGGING_HPP

#include <HaSLL/Logger.hpp>
#include <Nonempty/Pointer.hpp>

/**
 * @brief `noexcept` wrappers around HaSLL
 */

namespace Technology_Adapter::Modbus::Logging {

template <class... Args>
void log(Nonempty::Pointer<HaSLL::LoggerPtr> logger, HaSLL::SeverityLevel,
    Args&&... args) noexcept;

template <class... Args>
void trace(Nonempty::Pointer<HaSLL::LoggerPtr> logger, Args&&... args) noexcept;

template <class... Args>
void debug(Nonempty::Pointer<HaSLL::LoggerPtr> logger, Args&&... args) noexcept;

template <class... Args>
void error(Nonempty::Pointer<HaSLL::LoggerPtr> logger, Args&&... args) noexcept;

} // namespace Technology_Adapter::Modbus::Logging

#include "../implementations/Logging__implementation.hpp"

#endif //_MODBUS_TECHNOLOGY_ADAPTER_LOGGING_HPP
