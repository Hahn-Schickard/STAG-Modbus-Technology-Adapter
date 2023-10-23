#include "internal/ThreadsafeStrerror.hpp"

#include <cstring>
#include <new>

namespace Errno {

// @throws `std::bad_alloc`
ConstString constString(std::string const& source) {
  size_t size = (source.size() + 1) * sizeof(char);
  char* copy = (char*)operator new(size);
  memcpy(copy, source.c_str(), size);
  return ConstString(copy);
}

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
      char const* in_message = strerror(errnum);
      if (in_message == nullptr) {
        in_message = "strerror returned nullptr";
      }
      std::string out_message(in_message);
      out_message += " (code ";
      out_message += std::to_string(errnum);
      out_message += ")";
      return constString(out_message);
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
      return constString(out_message);
    } catch (...) {
      return out_of_memory_message;
    }
  }
}

} // namespace Errno
