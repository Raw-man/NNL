/**
 * @file panic.hpp
 * @brief Provides functions to set up the callback for fatal library errors.
 *
 * @see nnl::SetGlobalPanicCB
 *
 */
#pragma once

#include <string_view>

namespace nnl {

/**
 * \defgroup Panic Panic
 * @ingroup Common
 * @copydoc panic.hpp
 *
 * @{
 */

using PanicFun = void (*)(std::string_view reason) noexcept;

/**
 * @brief Sets the global callback for fatal library errors.
 *
 * This callback is invoked by nnl::Panic() whenever the library encounters an
 * unrecoverable state. This occurs in two cases:
 * 1. A contract violation (e.g., NNL_EXPECTS fails).
 * 2. An exception is thrown but the library is compiled without exception support.
 *
 * @param callback The function to be executed for custom error reporting or
 *        cleanup before the process terminates.
 *
 * @see NNL_EXPECTS
 */
void SetGlobalPanicCB(PanicFun callback) noexcept;

[[noreturn]] void Panic(std::string_view reason) noexcept;

/** @} */
}  // namespace nnl
