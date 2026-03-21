/**
 * @file exception.hpp
 * @brief Defines the library-wide exception hierarchy and error handling macros.
 *
 */

#pragma once

#include <string>

#include "NNL/common/src_info.hpp"

/**
 * \defgroup Exceptions Exceptions
 * @ingroup Common
 * @copydoc exception.hpp
 *
 * @{
 */

/**
 * @def NNL_THROW
 * @brief Throws an exception or aborts the program if exceptions are disabled
 *
 * @param object The exception object to be thrown
 */
/**
 * @def NNL_TRY
 * @brief Expands to a try statement; or a dummy if statement when
 * exceptions are disabled.
 */
/**
 * @def NNL_CATCH
 * @brief Expands to a catch statement; or a dummy if statement when
 * exceptions are disabled.
 *
 * @param type The unqualified non-pointer type to catch
 */
#if (defined(__cpp_exceptions) || defined(__EXCEPTIONS) || defined(_CPPUNWIND))
#define NNL_THROW(object) throw object
#define NNL_TRY try
#define NNL_CATCH(type) catch (const type& e)
#else
#include "NNL/common/logger.hpp"  // IWYU pragma: export
#ifdef NNL_LOG_ENABLED
#define NNL_THROW(object)                           \
  {                                                 \
    nnl::Log(object.what(), nnl::LogLevel::kError); \
    std::terminate();                               \
  }
#else
#define NNL_THROW(object) \
  { std::terminate(); }
#endif
#define NNL_TRY if (true)

#define NNL_CATCH(type) else if (type e; false)
#endif

#define NNL_ERMSG(msg) ((msg) + (" [" + NNL_SRCINF + "]"))
/** @} */

namespace nnl {

/**
 * \addtogroup Exceptions
 * @{
 */

/**
 * @brief Base exception class for the library specific exceptions
 */
class Exception : public std::exception {
 public:
  Exception(std::string what_arg = "") : m_(std::move(what_arg)) {}

  const char* what() const noexcept override { return m_.c_str(); }

  virtual ~Exception() = default;

 protected:
  static std::string Name(const std::string& ename) { return "[nnl." + ename + "] "; }
  const std::string m_;
};
/**
 * @class RuntimeError
 * @brief Exception thrown for generic runtime errors
 */
class RuntimeError : public Exception {
 public:
  RuntimeError(const std::string& what_arg = "") : Exception(Exception::Name("runtime_error") + what_arg) {}
};
/**
 * @class ParseError
 * @brief Exception thrown for parsing errors
 */
class ParseError : public Exception {
 public:
  ParseError(const std::string& what_arg = "") : Exception(Exception::Name("parse_error") + what_arg) {}
};
/**
 * @class PreconditionError
 * @brief Exception thrown when a contract precondition is violated.
 *
 * Usually signals that a function
 * received invalid input or was called in an invalid state.
 *
 * @see NNL_EXPECTS
 */
class PreconditionError : public Exception {
 public:
  PreconditionError(const std::string& what_arg = "") : Exception(Exception::Name("precondition_error") + what_arg) {}
};
/**
 * @class RangeError
 * @brief Exception thrown for out-of-range errors
 */
class RangeError : public Exception {
 public:
  RangeError(const std::string& what_arg = "") : Exception(Exception::Name("range_error") + what_arg) {}
};
/**
 * @class NarrowError
 * @brief Exception thrown for narrowing conversion errors
 *
 * @see nnl::utl::data::narrow
 */
class NarrowError : public Exception {
 public:
  NarrowError(const std::string& what_arg = "") : Exception(Exception::Name("narrow_error") + what_arg) {}
};
/**
 * @class IOError
 * @brief Exception thrown for I/O like operations
 *
 * @see nnl::RWBase
 */

class IOError : public Exception {
 public:
  IOError(const std::string& what_arg = "") : Exception(Exception::Name("io_error") + what_arg) {}

  IOError(const std::string& what_arg, const std::string& path)
      : Exception(Exception::Name("io_error") + path + ": " + what_arg) {}
};
/** @} */
}  // namespace nnl
