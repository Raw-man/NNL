/**
 * @file visanimation.hpp
 * @brief Contains functions and structures for working with "sub" animations
 * that control the visibility of mesh groups during the playback of main SRT
 * animations.
 *
 * @see nnl::visanimation::AnimationContainer
 * @see nnl::visanimation::IsOfType
 * @see nnl::visanimation::Import
 * @see nnl::visanimation::Export
 * @see nnl::visanimation::Convert
 * @see nnl::model::Model::mesh_groups
 * @see nnl::animation::AnimationContainer
 */
#pragma once

#include <map>
#include <vector>

#include "NNL/common/fixed_type.hpp"
#include "NNL/common/io.hpp"
#include "NNL/simple_asset/sanimation.hpp"
namespace nnl {
/**
 * \defgroup Subanimation Visibility Animation
 * @ingroup Visual
 * @copydoc visanimation.hpp
 * @{
 */
/**
 * @copydoc visanimation.hpp
 *
 */
namespace visanimation {
/**
 * \defgroup Subanimation_Main Main
 * @ingroup Subanimation
 * @{
 */
/**
 * @brief Represents a keyframe in a visibility animation.
 *
 * This struct encapsulates the time at which the keyframe occurs in the
 * animation timeline, as well as flags that enable specific mesh groups.
 *
 * @see nnl::visanimation::Animation
 */
struct KeyFrame {
  u16 time_tick = 0;                             ///< Time in ticks. This value must be less than the duration
                                                 ///< of the respective main animation
  u16 flags_enable_groups = 0b0000000000000000;  ///< A set of 16 bit flags that specify which mesh
                                                 ///< groups should be enabled (a value of 0 does **not**
                                                 ///< disable them)
};
/**
 * @brief Represents a single visibility animation.
 *
 * This struct represents a "sub" animation that is played along the main SRT
 * animation and that controls the visibility of mesh groups. This struct contains
 * two channels of keyframes that _may_ be associated with the left and the
 * right hands of a character. The values from those channels are combined together to determine
 * the visibility status of a mesh group.
 *
 * @see nnl::visanimation::AnimationContainer
 */
struct Animation {
  std::vector<KeyFrame> left_channel;   ///< Keyframes for the left channel of the animation.
  std::vector<KeyFrame> right_channel;  ///< Keyframes for the right channel of the animation.
};
/**
 * @brief  Holds a collection of visibility animations.
 *
 * This struct serves as a container for multiple animations that control the
 * visibility of mesh groups during the playback of main SRT animations.
 *
 * When a model is spawned, all mesh groups are enabled by default. As keyframes
 * are encountered, the following rules apply:
 * - All groups specified in the disable flags are deactivated.
 * - All groups specified in the enable flags of a keyframe are reactivated.
 * - If a group is not included in the disable flags, it always remains visible.
 * - When a keyframe with enable flags set to all 0's is encountered, the two
 * groups (for both left and right channels) that were set to 1 in the lowest bits of the
 * disable flags are re-enabled anyway.
 * - The resulting values from the right and left channels are combined using
 *   a bitwise OR operation.
 *
 * @note This animation can also be (rarely) controlled from an action configuration or
 * can be hardcoded.\n\n
 *
 * @note If visibility animation is used, the number of animations in this container
 * must match the number of regular SRT animations.\n\n
 *
 * @note This struct stores only a portion of the data that may be
 * required by a 3D asset to function properly. Its binary representation must
 * be used as part of a larger container that **must** also include regular SRT
 * animations and possibly other data.
 *
 * @see nnl::asset::Asset
 * @see nnl::asset::Asset3D
 * @see nnl::model::Model
 * @see nnl::animation::AnimationContainer
 * @see nnl::action::ActionConfig
 */
struct AnimationContainer {
  u16 flags_disable_left_channel = 0b0000000000000000;   ///< flags to disable the mesh groups for keyframes
                                                         ///< of the left channel. The value of 1 disables
                                                         ///< them, 0 does nothing.
  u16 flags_disable_right_channel = 0b0000000000000000;  ///< flags to disable the mesh groups for keyframes
                                                         ///< of the right channel. The value of 1 disables
                                                         ///< them, 0 does nothing.
  std::vector<Animation> animations;                     ///< A collection of visibility "sub" animations. The size of
                                                         ///< this vector must match the number of main SRT
                                                         ///< animations.
};

/**
 * @brief Converts multiple simple visibility animations to the in-game format.
 *
 * This function takes a vector of `SVisibilityAnimation` objects and converts
 * them into an `AnimationContainer`.
 *
 * @param sanimations The vector of animations to be converted.
 * @return AnimationContainer The converted container of in-game visibility
 * animations.
 *
 * @note Use `std::move` when passing an existing object.
 */
AnimationContainer Convert(std::vector<SVisibilityAnimation>&& sanimations);

/**
 * @brief Converts multiple in-game visibility animations to a simpler format.
 *
 * This function takes an `AnimationContainer` object and converts all
 * contained animations into a format suitable for exporting to other formats.
 *
 * @param animation_container The container of in-game animations to be
 * converted.
 * @return A vector of converted animations.
 */
std::vector<SVisibilityAnimation> Convert(const AnimationContainer& animation_container);

/**
 * @brief Tests if the provided file is a visibility animation.
 *
 * This function takes data representing a file and checks
 * whether it corresponds to the in-game visibility animation format.
 *
 * @param buffer The binary file to be tested.
 * @return Returns true if the file is identified as a animation container.
 *
 * @see nnl::visanimation::Import
 */
bool IsOfType(BufferView buffer);

bool IsOfType(const std::filesystem::path& path);

bool IsOfType(Reader& f);

/**
 * @brief Imports animations from a binary file.
 *
 * This function takes a file buffer, parses its contents, and returns an
 * `AnimationContainer`.
 *
 * @param buffer The binary file to be imported.
 * @return AnimationContainer The imported container of animations.
 *
 * @see nnl::visanimation::IsOfType
 * @see nnl::visanimation::Export
 */
AnimationContainer Import(BufferView buffer);

AnimationContainer Import(const std::filesystem::path& path);

AnimationContainer Import(Reader& f);
/**
 * @brief Exports animations to a binary file format.
 *
 * This function takes an `AnimationContainer` and exports it into a binary file
 * buffer.
 *
 * @param animation_container The container of animations to be exported.
 * @return The exported animations in the binary format.
 */
[[nodiscard]] Buffer Export(const AnimationContainer& animation_container);

void Export(const AnimationContainer& animation_container, const std::filesystem::path& path);

void Export(const AnimationContainer& animation_container, Writer& f);
/** @} */

/**
 * \defgroup Subanimation_Auxiliary Auxiliary
 * @ingroup Subanimation
 * @{
 */

/**
 * @brief Converts a simple animation format to an in-game visibility animation.
 *
 * This function takes an `SVisibilityAnimation` object and converts it into the
 * in-game format.
 *
 * @param sanimation The animation to be converted.
 * @return Animation The converted in-game visibility animation.
 *
 * @note Use `std::move` when passing an existing object.
 */
Animation Convert(SVisibilityAnimation&& sanimation);

/**
 * @brief Converts an in-game animation to a simpler representation.
 *
 * This function takes an `Animation` object and converts it into a format
 * that is more suitable for exporting to other formats.
 *
 * @param animation The in-game animation to be converted.
 * @param flags_disable_left The mesh groups that get disabled as keyframes are encountered
 * @param flags_disable_right The mesh groups that get disabled as keyframes are encountered
 * @return SVisibilityAnimation The converted animation in a simpler
 * format.
 */
SVisibilityAnimation Convert(const Animation& animation, u16 flags_disable_left, u16 flags_disable_right);
/** @} */

namespace raw {
/**
 * \defgroup Subanimation_Raw Raw
 * @ingroup Subanimation
 * @{
 */
constexpr u16 kMagicBytes = 0;  ///< Magic bytes (?)

NNL_PACK(struct RHeader {
  u16 magic_bytes = kMagicBytes;
  u16 num_animations = 0;
  u16 flags_disable_left_channel = 0b0000000000000000;
  u16 flags_disable_right_channel = 0b0000000000000000;
});

static_assert(sizeof(RHeader) == 0x8);

NNL_PACK(struct RAnimation {
  u32 offset_keyframes = 0;
  u16 num_left_channel_keyframes = 0;
  u16 num_right_channel_keyframes = 0;
});

static_assert(sizeof(RAnimation) == 0x8);

NNL_PACK(struct RKeyFrame {
  u16 time_tick = 0;
  u16 flags_enable_groups = 0b0000000000000000;
});

static_assert(sizeof(RKeyFrame) == 0x4);

struct RAnimationContainer {
  RHeader header;
  std::vector<RAnimation> animations;

  // Maps of offsets (used in the structures above) to their data:
  std::map<u32, std::vector<RKeyFrame>> keyframes;
};

AnimationContainer Convert(const RAnimationContainer& rsubanim);

RAnimationContainer Parse(Reader& f);
/** @} */
}  // namespace raw
}  // namespace visanimation
/** @} */
}  // namespace nnl
