/**
 * @file constant.hpp
 * @brief Global configuration constants for the library.
 * 
 */
#pragma once
/**
 * \defgroup Constants Constants
 * @ingroup Common
 * @copydoc constant.hpp
 *
 * @{
 */

/**
 * @def NNL_ALPHA_TRANSP
 * @brief Alpha values <= the threshold are considered fully transparent
 */
#ifndef NNL_ALPHA_TRANSP
#define NNL_ALPHA_TRANSP (5)
#endif

/**
 * @def NNL_ALPHA_OPAQ
 * @brief Alpha values >= the threshold are considered fully opaque
 */
#ifndef NNL_ALPHA_OPAQ
#define NNL_ALPHA_OPAQ (250)
#endif

/**
 * @def NNL_ALPHA_TRANSP_F
 * @brief Alpha values <= the threshold are considered fully transparent
 */
#ifndef NNL_ALPHA_TRANSP_F
#define NNL_ALPHA_TRANSP_F (0.02f)
#endif

/**
 * @def NNL_ALPHA_OPAQ_F
 * @brief Alpha values >= the threshold are considered fully opaque
 */
#ifndef NNL_ALPHA_OPAQ_F
#define NNL_ALPHA_OPAQ_F (0.98f)
#endif

/**
 * @def NNL_EPSILON
 * @brief Default floating-point comparison epsilon
 */
#ifndef NNL_EPSILON
#define NNL_EPSILON (1.0e-5f)
#endif

/**
 * @def NNL_EPSILON2
 * @brief Default squared epsilon
 * @see NNL_EPSILON
 */
#ifndef NNL_EPSILON2
#define NNL_EPSILON2 ((NNL_EPSILON) * (NNL_EPSILON))
#endif

/** @} */

