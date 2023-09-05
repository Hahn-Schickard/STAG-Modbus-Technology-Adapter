#ifndef _MODBUS_TECHNOLOGY_ADAPTER_THREADSAFE_STRERROR_HPP
#define _MODBUS_TECHNOLOGY_ADAPTER_THREADSAFE_STRERROR_HPP

/**
 * @brief Thread-safety for functions like `std::strerror`
 *
 * Notoriously, `std::strerror` is not thread-safe (in contrast to `errno` which
 * is thread-local) and there is no standard thread-safe version of it. Also,
 * some libraries provide their own extension of `errno` semantics together with
 * their own version of `stderror`.
 *
 * This module provides a thread-safe version of `std::strerror` as well as
 * support for making variants of `strerror` thread-safe.
 *
 * Of course, thread-safety works only if ALL access to `strerror`-like
 * functionality happens through this module.
 */

#include <memory>
#include <mutex>

#include <Nonempty_Pointer/NonemptyPtr.hpp>
#include <Threadsafe_Containers/SharedPtr.hpp>

namespace Errno {

/**
 * A read-only memory-managed string
 *
 * This functionality appears to be missing from the standard library.
 * In contrast to `std::string`, copying this type requires neither allocation
 * nor copying the contents.
 * In contrast to `std::string_view`, we have memory management.
 */
using ConstString =
    NonemptyPointer::NonemptyPtr<Threadsafe::SharedPtr<char const>>;

/**
 * @brief Threadsafe C++-style version of `std::strerror`
 *
 * Equivalent to `generic(std::strerror, errnum)`
 *
 * @pre The current thread does not hold `strerror_mutex`
 * @post The current thread does not hold `strerror_mutex`
 */
ConstString strerror(int errnum) noexcept;

/**
 * @brief Wrapper around `strerror` for thread-safety and C++ style
 *
 * @pre `strerror` does not throw
 * @pre The current thread does not hold `strerror_mutex`
 * @post The current thread does not hold `strerror_mutex`
 */
ConstString generic_strerror(
    std::function<char const*(int)> const& strerror, int errnum) noexcept;

/**
 * @brief Mutex for calls to `strerror` or similar.
 *
 * This mutex may be used to protect custom `stderror` functions, like those
 * from libraries that extend the `errno` semantics.
 */
static std::mutex strerror_mutex;

} // namespace Errno

#endif // _MODBUS_TECHNOLOGY_ADAPTER_THREADSAFE_STRERROR_HPP
