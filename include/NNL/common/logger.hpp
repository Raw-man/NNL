/**
 * @file logger.hpp
 * @brief Provides functions to set up logging.
 *
 * @see nnl::SetGlobalLogCB
 *
 */
#pragma once

#include <string_view>

/**
 * \defgroup Logger Logger
 * @ingroup Common
 * @copydoc logger.hpp
 *
 * @{
 */

#ifdef NNL_ENABLE_LOGGING
#include "NNL/common/src_info.hpp"
#define NNL_LOG_WARN(msg)                            \
  do {                                               \
    nnl::Log(NNL_SRCTAG(msg), nnl::LogLevel::kWarn); \
  } while (false)
#define NNL_LOG_ERROR(msg)                            \
  do {                                                \
    nnl::Log(NNL_SRCTAG(msg), nnl::LogLevel::kError); \
  } while (false)
#else
#define NNL_LOG_WARN(msg) \
  do {                    \
  } while (false)
#define NNL_LOG_ERROR(msg) \
  do {                     \
  } while (false)
#endif
/** @} */

namespace nnl {
/**
 * @addtogroup Logger
 *
 * @{
 */

/**
 * @brief Enum class representing log levels.
 */
enum class LogLevel {
  /**
   * @brief Indicates error conditions.
   *
   * Further execution is likely not possible, and immediate attention is
   * required.
   */
  kError,
  /**
   * @brief Indicates warning conditions.
   *
   * Something is not right, and the user may want to take action to mitigate
   * potential issues.
   */
  kWarn
};
/**
 * @brief Type definition for a logging function.
 *
 * @param msg The log message to be processed.
 * @param log_level The severity level of the log message.
 *
 * @see nnl::SetGlobalLogCB
 */
using LogFun = void (*)(std::string_view msg, LogLevel log_level);

/**
 * @brief Sets the logging callback function.
 *
 * @param callback The function to be used for logging messages.
 *
 * @note To enable log messages, ensure that the library is compiled
 * with the NNL_ENABLE_LOGGING option set. If logging is invoked from multiple
 * threads, it is the user's responsibility to implement appropriate
 * synchronization mechanisms.
 *
 * @see nnl::LogFun
 */
void SetGlobalLogCB(LogFun callback) noexcept;

void Log(std::string_view msg, LogLevel log_lvl);

/** @} */
}  // namespace nnl
