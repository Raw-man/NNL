/**
 * @file model_common.hpp
 * @brief Contains model-related definitions that are shared across
 * different parts of the library.
 *
 * @see nnl::model::Model
 * @see nnl::colbox::CollisionBox14
 *
 */
#pragma once
#include "NNL/common/fixed_type.hpp"

namespace nnl {
/**
 * \defgroup Model Model
 * @ingroup Visual
 * @copydoc model.hpp
 * @{
 */

namespace model {
/**
 * \defgroup Model_Shared Shared
 * @ingroup Model
 * @copydoc model_common.hpp
 * @{
 */

/**
 * @brief Represents a concrete bone index in a skeleton/bone array.
 *
 * @see nnl::model::Model::skeleton
 */
using BoneIndex = u16;
/**
 * @brief Represents a generic bone identifier.
 *
 * A bone target is a generic bone id that can be mapped to a concrete bone in
 * the skeleton. This allows the games to interact with specific body parts
 * (e.g., "Right Hand" or "Head") regardless of the model's actual hierarchy.
 *
 * For example, these IDs are used to:
 * - Determine where to attach effects (e.g., Rasengan).
 * - Identify bones for procedural animation (like moving a character's head).
 * - Attach collision boxes for specific limbs.
 *
 * @see nnl::model::HumanoidRigTarget
 * @see nnl::model::Model::bone_target_tables
 * @see nnl::colbox::CollisionBox14
 */
using BoneTarget = u16;
/** @} */
}  // namespace model
/** @} */
}  // namespace nnl
