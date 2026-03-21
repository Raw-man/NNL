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
 * @note In debug builds, it uses assert; in release, it throws nnl::PreconditionError
 */
#ifdef NDEBUG
#include "NNL/common/exception.hpp"
#define NNL_EXPECTS(precondition)                                                          \
  do {                                                                                     \
    if (!static_cast<bool>(precondition)) {                                                \
      NNL_THROW(nnl::PreconditionError(NNL_ERMSG("precondition failed: " #precondition))); \
    }                                                                                      \
  } while (false)
#else
#define NNL_EXPECTS(precondition) \
  do {                            \
    assert(precondition);         \
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
#ifdef NDEBUG
#define NNL_ENSURES_DBG(postcondition) \
  do {                                 \
  } while (false)
#else
#define NNL_ENSURES_DBG(postcondition) \
  do {                                 \
    assert(postcondition);             \
  } while (false)
#endif

/** @} */
