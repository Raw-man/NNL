/**
 * @file contract.hpp
 * @brief Defines macros for Design-by-Contract verification.
 *
 * @see NNL_EXPECTS
 * @see NNL_EXPECTS_DBG
 * @see NNL_ENSURES_DBG
 */
#pragma once

#include <cassert>
/**
 * \defgroup Contracts Contracts
 * @ingroup Common
 * @copydoc contract.hpp
 *
 * @{
 */

/**
 * @def NNL_EXPECTS
 * @brief A precondition check.
 *
 * This macro is used to check requirements that must hold true before the function is called. The caller must
 * ensure these conditions; failure typically indicates an error in the calling code.
 *
 * @note By default, this macro terminates execution via a configurable callback.
 * If the CMake option @c NNL_THROW_ON_CONTRACT_VIOLATION is enabled,
 * it throws nnl::PreconditionError instead.
 *
 * @see nnl::SetGlobalPanicCB
 */
#ifdef NNL_THROW_ON_CONTRACT_VIOLATION
#include "NNL/common/exception.hpp"
#define NNL_EXPECTS(precondition)                                                          \
  do {                                                                                     \
    if (!static_cast<bool>(precondition)) {                                                \
      NNL_THROW(nnl::PreconditionError(NNL_ERMSG("precondition failed: " #precondition))); \
    }                                                                                      \
  } while (false)
#else
#include "NNL/common/panic.hpp"
#define NNL_EXPECTS(precondition)                                   \
  do {                                                              \
    if (!static_cast<bool>(precondition)) {                         \
      nnl::Panic(NNL_ERMSG("precondition failed: " #precondition)); \
    }                                                               \
  } while (false)
#endif

/**
 * @def NNL_EXPECTS_DBG
 * @brief Debug-only precondition check
 *
 * @see NNL_EXPECTS
 */
#define NNL_EXPECTS_DBG(precondition) \
  do {                                \
    assert(precondition);             \
  } while (false)

/**
 * @def NNL_ENSURES_DBG
 * @brief Debug-only postcondition check.
 *
 * This macro is used to check requirements that must hold true after the function executes.
 * The function itself is responsible for ensuring these conditions, assuming all preconditions
 * were met; failure indicates a bug in the function.
 */
#define NNL_ENSURES_DBG(postcondition) \
  do {                                 \
    assert(postcondition);             \
  } while (false)

/** @} */
