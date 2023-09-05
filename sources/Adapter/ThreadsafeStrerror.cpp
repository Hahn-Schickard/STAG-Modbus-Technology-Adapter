#include "ThreadsafeStrerror.hpp"

#include <cstring>
#include <new>

namespace Errno {

/*
  We alias to empty pointer, because the underlying memory is persistent.
*/
// There is no exception because the pointer is non-empty:
// NOLINTNEXTLINE(cert-err58-cpp)
ConstString const out_of_memory_message( //
    Threadsafe::SharedPtr<int>(nullptr),
    "Out of memory while trying to explain errno");

char const* system_error_prefix = "In strerror: System error during locking: ";

ConstString strerror(int errnum) noexcept {
  return generic_strerror(std::strerror, errnum);
}

ConstString generic_strerror(
    std::function<char const*(int)> const& strerror, int errnum) noexcept {

  try {
    std::lock_guard lock(strerror_mutex); // may throw std::system_error
    try {
      char const* message = strerror(errnum);
      if (message == nullptr) {
        message = "strerror returned nullptr";
      }
      size_t size = (std::strlen(message) + 1) * sizeof(char);
      char* copy = (char*)operator new(size);
      memcpy(copy, message, size);
      return ConstString(copy);
    } catch (...) {
      // Most of the above are C-style functions that don't throw.
      // What may have thrown is `new` or the `SharedPtr` constructor.
      return out_of_memory_message;
    }
  } catch (std::exception const& exception) {
    try {
      char const* message = exception.what();
      if (message == nullptr) {
        message = "exception::what returned nullptr";
      }
      size_t prefix_size = std::strlen(system_error_prefix) * sizeof(char);
      size_t message_size = (std::strlen(message) + 1) * sizeof(char);
      size_t total_size = prefix_size + message_size;
      void* copy = operator new(total_size);
      memcpy(copy, system_error_prefix, total_size);
      memcpy(copy, message, message_size);
      return ConstString((char const*)copy);
    } catch (...) {
      return out_of_memory_message;
    }
  }
}

} // namespace Errno
