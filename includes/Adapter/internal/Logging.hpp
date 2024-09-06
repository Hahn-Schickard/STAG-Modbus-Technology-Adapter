#ifndef _MODBUS_TECHNOLOGY_ADAPTER_LOGGING_HPP
#define _MODBUS_TECHNOLOGY_ADAPTER_LOGGING_HPP

#include <HaSLL/Logger.hpp>
#include <Nonempty_Pointer/NonemptyPtr.hpp>

/**
 * @brief `noexcept` wrappers around HaSLL
 */

namespace Technology_Adapter::Modbus::Logging {

template <class... Args>
void log(NonemptyPointer::NonemptyPtr<HaSLI::LoggerPtr> logger,
    HaSLI::SeverityLevel, Args&&... args) noexcept;

template <class... Args>
void trace(NonemptyPointer::NonemptyPtr<HaSLI::LoggerPtr> logger,
    Args&&... args) noexcept;

template <class... Args>
void debug(NonemptyPointer::NonemptyPtr<HaSLI::LoggerPtr> logger,
    Args&&... args) noexcept;

template <class... Args>
void error(NonemptyPointer::NonemptyPtr<HaSLI::LoggerPtr> logger,
    Args&&... args) noexcept;

} // namespace Technology_Adapter::Modbus::Logging

#include "../implementations/Logging__implementation.hpp"

#endif //_MODBUS_TECHNOLOGY_ADAPTER_LOGGING_HPP
