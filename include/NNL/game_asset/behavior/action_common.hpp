/**
 * @file action_common.hpp
 * @brief Contains the definition for action
 * identifiers that are used across different parts of the library.
 *
 * @see nnl::action::Id
 * @see nnl::action::ActionConfig
 * @see nnl::colbox::ColBoxConfig
 */
#pragma once
#include "NNL/common/fixed_type.hpp"
#include "NNL/utility/data.hpp"
namespace nnl {
/**
 * \defgroup Action Action
 * @ingroup Behavior
 * @copydoc action.hpp
 * @{
 */

namespace action {
/**
 * \defgroup Action_Shared Shared
 * @ingroup Action
 * @copydoc action_common.hpp
 * @{
 */
/**
 * @brief Defines the number of action categories;
 *
 * @see nnl::action::ActionCategory
 * @see nnl::action::kMaxNumActionsPerCat
 */
constexpr std::size_t kNumActionCategories = 4;

/**
 * @brief Defines the maximum number of actions per category;
 *
 * @see nnl::action::ActionCategory
 * @see nnl::action::kNumActionCategories
 */
constexpr std::size_t kMaxNumActionsPerCat = 0x3FFF;

/**
 * @brief Defines the categories of actions.
 *
 * Each category groups related actions with their animations and effects.
 *
 * @see nnl::action::Id
 */
enum class ActionCategory {
  kMovement = 0,  ///< Related to moving the entity.
  kCombat = 1,    ///< Related to attacking behaviors.
  kDamage = 2,    ///< Related to receiving damage.
  kCutscene = 3   ///< Related to scripted events or cutscenes.
};
/**
 * @brief Represents a unique identifier for an action.
 *
 * The `action_category` member determines the type of an action, while the
 * `action_index` provides a unique identifier for the action within that
 * category. Together, they form a complete identifier for an action.
 *
 * @see nnl::action::ActionConfig
 * @see nnl::colbox::ColBoxConfig
 */
struct Id {
  ActionCategory action_category = ActionCategory::kMovement;  ///< Type of the action
  u16 action_index = 0;  ///< The index within the category @see nnl::action::kMaxNumActionsPerCat
};

inline bool operator==(const Id& rhs, const Id& lhs) {
  return rhs.action_category == lhs.action_category && rhs.action_index == lhs.action_index;
  /** @} */
}
}  // namespace action
/** @} */
}  // namespace nnl

namespace std {
template <>
struct hash<nnl::action::Id> {
  size_t operator()(const nnl::action::Id& id) const {
    return hash<size_t>{}(nnl::utl::data::as_int(id.action_category) << 14 | id.action_index);
  }
};
}  // namespace std
