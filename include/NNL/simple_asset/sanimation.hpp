/**
 * @file sanimation.hpp
 * @brief Provides data structures for representing various animation types
 * and their essential components.
 *
 * This file defines structures for various animation types found in 3D models,
 * including skeletal animations, visibility animations, and texture coordinate
 * animations. These structures facilitate the conversion of animation data
 * between various common exchange formats (such as FBX and glTF) and in-game
 * formats.
 *
 * @see nnl::SAnimation
 * @see nnl::SVisibilityAnimation
 * @see nnl::SUVAnimation
 */
#pragma once

#include <array>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <map>
#include <string>
#include <vector>

#include "NNL/common/fixed_type.hpp"
#include "NNL/simple_asset/svalue.hpp"

namespace nnl {

/**
 * \defgroup SAnimation Simple Animation
 * @ingroup Simple3DAsset
 * @copydoc sanimation.hpp
 * @{
 */

/**
 * @brief Represents a single keyframe in an animation.
 *
 * This template structure holds the time at which the keyframe occurs in the
 * animation timeline along with a new value set at that time.
 *
 * @tparam T The type of value associated with the keyframe.
 *
 * @see nnl::SBoneChannel
 * @see nnl::SVisibilityChannel
 * @see nnl::SUVChannel
 */
template <typename T>
struct SKeyFrame {
  u16 time = 0;  ///< The timestamp in ticks (1 tick ~ 1 frame).
  T value{};     ///< The new value at the specified time.
};

/**
 * @brief Enumeration of interpolation modes for animations.
 *
 * This enum defines the modes of interpolation for keyframes in
 * animations.
 *
 * @note Currently, these modes are only applicable to UV animations.
 * For other animation types, the interpolation method is pre-defined and fixed.
 *
 * @see nnl::SUVAnimation
 */
enum class SInterpolationMode { kLinear, kConstant };

/**
 * @brief Represents the animation tracks for a single bone.
 *
 * This structure defines how a bone transforms over time during
 * animation.
 *
 * @note Linear interpolation of keyframes and a constant rate of 30 ticks per second are assumed.
 * Keyframes must be in ascending order and should not contain duplicate timestamps. Additionally,
 * the time values of keyframes must not exceed the duration of the animation they belong to.
 *
 * @see nnl::SAnimation
 */
struct SBoneChannel {
  std::vector<SKeyFrame<glm::vec3>> scale_keys;        ///< Scale transformations.
  std::vector<SKeyFrame<glm::quat>> rotation_keys;     ///< Rotation transformations
  std::vector<SKeyFrame<glm::vec3>> translation_keys;  ///< Translation transformations.
};
/**
 * @brief Represents a skeletal animation.
 *
 * This struct consists of channels that specify the transformations over time for
 * each bone of a skeleton.
 *
 * @see nnl::animation::Animation
 * @see nnl::SSkeleton
 * @see nnl::SModel
 * @see nnl::SAsset3D
 */
struct SAnimation {
  std::string name;  ///< An optional name of the animation.

  u16 duration = 0;  ///< Duration of the animation in ticks.
                     ///< It must be >= (last keyframe time + 1).

  std::vector<SBoneChannel> bone_channels;  ///< Animation channels for bones of the skeleton.
                                            ///< The size of this vector **must** match the total
                                            ///< number of bones in the skeleton.
                                            ///
  SValue extras;                            ///< Any additional data for custom use.

  /**
   * @brief Removes redundant keyframes.
   *
   * This function removes keyframes that are identical or may be calculated
   * via interpolation.
   *
   * @param tolerance The maximum allowed deviation from the original key value
   */
  void Unbake(float tolerance = 1.0e-4f);
  /**
   * @brief Bakes the animation.
   *
   * This method generates keyframes for every frame
   * of the animation, creating a complete set of keyframes.
   */
  void Bake();
};
/**
 * @brief A single animation track for visibility of a mesh group.
 *
 * @see nnl::SVisibilityAnimation
 */
using SVisibilityChannel = std::vector<SKeyFrame<bool>>;

/**
 * @brief Represents a visibility animation of mesh groups.
 *
 * This structure consists of channels that control the visibility of mesh
 * groups over time. Up to 16 groups are supported.
 *
 * @note Constant interpolation of keyframes and a constant rate of 30 ticks per second are assumed.
 * Keyframes must be in ascending order and should not contain duplicate timestamps. Unlike game
 * visibility animations, which may use 2 channels (left and right) for each mesh group, the
 * channels in this struct strictly correspond to 1 group, and the values in
 * them are absolute - there's no need for additional bitwise operations.
 *
 * @see nnl::visanimation::Animation
 * @see nnl::SMesh
 * @see nnl::SAsset3D
 */
struct SVisibilityAnimation {
  std::string name;                                        ///< An optional name for the animation
  std::array<SVisibilityChannel, 16> visibility_channels;  ///< Channels for 16 mesh groups
};
/**
 * @brief A single animation track for UV coordinates.
 *
 * @see nnl::SUVAnimation
 */
struct SUVChannel {
  SInterpolationMode interpolation;        ///< The method used to interpolate between keyframes.
  std::vector<SKeyFrame<glm::vec2>> keys;  ///< The keyframes.
};
/**
 * @brief Represents a texture coordinate animation.
 *
 * @note Constant or Linear interpolation of keyframes may be used. Keyframes
 * must be in ascending order and should not contain duplicate timestamps.\n
 * Unlike the UVAnimation config defined in the model namespace, this struct can
 * represent any offset transformations over time. Only a subset of these can be
 * converted into the UVAnimation structure.
 *
 * @see nnl::model::UVAnimation
 * @see nnl::SModel
 */
struct SUVAnimation {
  std::string name;  ///< An optional name for the animation

  std::map<std::size_t, SUVChannel> translation_channels;  ///< A map where the key is the id of the target
                                                           ///< material and the value is an animation
                                                           ///< channel that controls UV offsets of the
                                                           ///< material's texture.
  SValue extras;                                           ///< Any additional data for custom use.
};
/** @} */
}  // namespace nnl
