/**
 * @file animation.hpp
 * @brief Contains functions and structures for working with in-game skeletal
 * animations.
 *
 * @see nnl::animation::AnimationContainer
 * @see nnl::animation::IsOfType
 * @see nnl::animation::Import
 * @see nnl::animation::Export
 * @see nnl::animation::Convert
 *
 */
#pragma once

#include "NNL/common/fixed_type.hpp"
#include "NNL/common/io.hpp"
#include "NNL/simple_asset/sanimation.hpp"

namespace nnl {
/**
 * \defgroup Animation Skeletal Animation
 * @ingroup Visual
 * @copydoc animation.hpp
 * @{
 */
/**
 * @copydoc animation.hpp
 *
 */
namespace animation {
/**
 * \defgroup Animation_Main Main
 * @ingroup Animation
 * @{
 */

/**
 * @brief Represents a keyframe in an animation.
 *
 * This struct encapsulates the time at which the keyframe occurs in the
 * animation timeline, as well as a new value set at that keyframe.
 */
struct KeyFrame {
  u16 time{0};            ///< Time in ticks (1 tick ~ 1 frame). @note In few invalid assets,
                          ///< this value exceeds the total duration, in
                          ///< which case an animation gets truncated.
  glm::vec3 value{0.0f};  ///< The new value at the specified time. This can store scale,
                          ///< translation, or Euler rotation (pitch, yaw, roll) in degrees.
                          ///< Note that rotation values must be in the range [-180, 180].
};

/**
 * @brief Represents the transformations of a bone during an animation.
 *
 * This struct contains keyframes that set new scale, rotatation, and
 * translation transformations of a bone.
 *
 * @note Each vector must contain at least 1 keyframe.
 */
struct BoneChannel {
  std::vector<KeyFrame> scale_keys;        ///< Scale keys
  std::vector<KeyFrame> rotation_keys;     ///< Rotation keys
  std::vector<KeyFrame> translation_keys;  ///< Translation keys
};
/**
 * @brief Represents a single skeletal animation.
 *
 * @see nnl::animation::AnimationContainer
 */
struct Animation {
  u16 duration = 1;                             ///< The duration of the animation in ticks. In general, it should be
                                                ///< set to the time of the last keyframe + 1.
  std::vector<BoneChannel> animation_channels;  ///< Bone animation data for each bone of a skeleton.
                                                ///< @note The number of channels must match the
                                                ///< number of bones in the model. @see
                                                ///< nnl::model::Model::skeleton
};
/**
 * @brief Holds a collection of skeletal animations.
 *
 * @note This struct usually stores only a portion of the data that may be
 * required by a 3D asset to function properly. Its binary representation must
 * be used as part of a larger container that may also include models,
 * textures and other data.
 *
 * @see nnl::asset::Asset
 * @see nnl::asset::Asset3D
 * @see nnl::model::Model
 * @see nnl::action::ActionConfig
 * @see nnl::visanimation::AnimationContainer
 */
struct AnimationContainer {
  bool move_with_root = false;        ///< Doesn't affect anything (?). It reflects the value of the
                                      ///< same flag from the Model struct @see nnl::model::Model
  std::vector<Animation> animations;  ///< Collection of animations.
};

/**
 * @brief Converts multiple in-game animations to a simpler format.
 *
 * This function takes an `AnimationContainer` object and converts all
 * contained animations into a format that is more suitable for exporting to other formats.
 *
 * It performs the following data normalization:
 * - Transforms Euler angles into quaternions.
 * - Re-bases animations to start at frame 0.
 * - Properly clips keyframes to ensure they do not exceed the defined animation duration.
 *
 * @param animations The container of in-game animations to be converted.
 * @return A vector of converted animations in a simpler format.
 */
std::vector<SAnimation> Convert(const AnimationContainer& animations);
/**
 * @brief Parameters for converting animations to the in-game format.
 *
 *
 * @see nnl::animation::Convert
 * @see nnl::SAnimation
 */
struct ConvertParam {
  bool unbake = true;  ///< Remove keyframes that are duplicate or can be derived via interpolation.
};

/**
 * @brief Converts multiple animations back to AnimationContainer.
 *
 * This function takes a vector of `SAnimation` objects and converts them
 * into an `AnimationContainer` object that represent the in-game animation format.
 *
 * @param sanimations The vector of animations to be converted.
 * @param anim_params Conversion parameters
 * @param move_with_root Sets the respective value to true or false. Has no noticeable effect.
 * @return The converted container of in-game animations.
 *
 * @note Use `std::move` when passing an existing object.
 */
AnimationContainer Convert(std::vector<SAnimation>&& sanimations, const ConvertParam& anim_params = {},
                           bool move_with_root = false);

AnimationContainer Convert(std::vector<SAnimation>&& sanimations, const std::vector<ConvertParam>& anim_params,
                           bool move_with_root = false);

/**
 * @brief Tests if the provided file is an animation container.
 *
 * This function takes a file buffer and checks
 * whether it corresponds to the in-game animation format.
 *
 * @param buffer The binary file to be tested.
 * @return Returns true if the file is identified as an animation container.
 *
 * @see nnl::animation::Import
 */
bool IsOfType(BufferView buffer);

bool IsOfType(const std::filesystem::path& path);

bool IsOfType(Reader& f);

/**
 * @brief Imports animations from a binary file.
 *
 * This function takes a file buffer, parses its contents, and returns an
 * `AnimationContainer` object.
 *
 * @param buffer The binary file to be imported.
 * @return AnimationContainer The imported container of animations.
 *
 * @see nnl::animation::IsOfType
 * @see nnl::animation::Export
 */
AnimationContainer Import(BufferView buffer);

AnimationContainer Import(const std::filesystem::path& path);

AnimationContainer Import(Reader& f);
/**
 * @brief Exports animations to a binary file format.
 *
 * This function takes an `AnimationContainer` and exports it into the binary format.
 *
 * @param animation_container The container of animations to be exported.
 * @return The exported animations in the binary format.
 */
[[nodiscard]] Buffer Export(const AnimationContainer& animation_container);

void Export(const AnimationContainer& animation_container, const std::filesystem::path& path);

void Export(const AnimationContainer& animation_container, Writer& f);
/** @} */  // Main

/**
 * \defgroup Animation_Auxiliary Auxiliary
 * @ingroup Animation
 * @{
 */

/**
 * @brief Converts an in-game animation to a simpler representation.
 *
 * This function takes an `Animation` object and converts it into a format
 * that is more suitable for exporting to other formats.
 *
 * @param animation The in-game animation to be converted.
 * @return The converted animation in a simpler format.
 */
SAnimation Convert(const Animation& animation);

/**
 * @brief Converts a simple animation format back to an in-game animation.
 *
 * This function takes an `SAnimation` object and converts it into the in-game
 * format.
 *
 * @param sanimation The animation to be converted.
 * @param anim_param Conversion parameters.
 * @return The converted in-game animation.
 *
 * @note Use `std::move` when passing an existing object.
 */
Animation Convert(SAnimation&& sanimation, const ConvertParam& anim_param = {});
/** @} */  // Auxiliary

namespace raw {
/**
 * \defgroup Animation_Raw Raw
 * @ingroup Animation
 *
 * The `raw` namespace contains structures that define
 * the binary representation of animation data and functions to work with it.
 * This includes low-level data structures that are used to serialize and
 * deserialize animations directly from binary files.
 *
 * The detailed representations in the raw namespace are abstracted in the main
 * `animation` namespace to provide a more user-friendly interface.
 * @{
 */

/**
 * @brief Magic bytes used to identify the animation format.
 *
 * This constant represents a unique identifier that animation files start with.
 */
constexpr u32 kMagicBytes = 0x86'00'00'01;

NNL_PACK(struct RHeader {
  u32 magic_bytes = kMagicBytes;          // 0x0
  u16 num_animations = 0;                 // 0x4
  u16 num_bones_per_animation = 0;        // 0x6
  u16 move_with_root = 0;                 // 0x8
  u16 padding = 0;                        // 0xA
  u32 offset_duration_table = 0;          // 0xC
  u32 offset_keyframe_table = 0;          // 0x10
  u32 offset_rotation_table = 0;          // 0x14
  u32 offset_scale_traslation_table = 0;  // 0x18
});

static_assert(sizeof(RHeader) == 0x1C);

NNL_PACK(struct RBoneChannel {
  u32 index_keyframe_scale = 0;        // 0x0 index to keyframe table
  u32 index_keyframe_translation = 0;  // 0x8
  u32 index_keyframe_rotation = 0;     // 0x4
  u32 index_scale_table = 0;           // 0xC
  u32 index_rotation_table = 0;        // 0x10
  u32 index_translation_table = 0;     // 0x14 uses the same table as scale values
  u16 num_scale_transforms = 0;        // 0x18 how many subsequent keys to play
  u16 num_rotation_transforms = 0;     // 0x1A even if 0, the value at the index is still used
  u16 num_translation_transforms = 0;  // 0x1C
  u16 padding = 0;                     // padding
});

static_assert(sizeof(RBoneChannel) == 0x20);

struct RAnimationContainer {
  RHeader header;
  std::vector<RBoneChannel> bone_animations;
  std::vector<Vec3<f32>> scale_translation_table;
  std::vector<Vec3<i16>> rotation_table;  // normalized ints
  std::vector<u16> keyframe_table;
  std::vector<u16> duration_table;
};

AnimationContainer Convert(const RAnimationContainer& ranimation_container);

RAnimationContainer Parse(Reader& f);
/** @} */
}  // namespace raw
}  // namespace animation
/** @} */
}  // namespace nnl
