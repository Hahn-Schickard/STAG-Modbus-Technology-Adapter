#include "internal/ThreadsafeStrerror.hpp"

#include <cstring>
#include <new>

namespace Errno {

/*
  There is only one way that the following may throw: Right at the start of the
  program, there is not enough memory for our message. In that case we have a
  more serious problem than an exception that cannot be caught.
  Hence:
*/
// NOLINTNEXTLINE(cert-err58-cpp)
ConstString::ConstString const out_of_memory_message{
    "Out of memory while trying to explain errno"};

char const* system_error_prefix = "In strerror: System error during locking: ";

ConstString::ConstString strerror(int errnum) noexcept {
  return generic_strerror(std::strerror, errnum);
}

ConstString::ConstString generic_strerror(
    std::function<char const*(int)> const& strerror, int errnum) noexcept {

  try {
    std::lock_guard lock(strerror_mutex); // may throw std::system_error
    try {
      char const* in_message = strerror(errnum);
      if (in_message == nullptr) {
        in_message = "strerror returned nullptr";
      }
      std::string out_message(in_message);
      out_message += " (code ";
      out_message += std::to_string(errnum);
      out_message += ")";
      return ConstString::ConstString(out_message);
    } catch (...) {
      return out_of_memory_message;
    }
  } catch (std::exception const& exception) {
    try {
      char const* in_message = exception.what();
      if (in_message == nullptr) {
        in_message = "exception::what returned nullptr";
      }
      std::string out_message(system_error_prefix);
      out_message += in_message;
      return ConstString::ConstString(out_message);
    } catch (...) {
      return out_of_memory_message;
    }
  }
}

} // namespace Errno
