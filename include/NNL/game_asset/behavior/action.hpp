/**
 * @file action.hpp
 * @brief Contains structures and functions for working with game actions.
 *
 * @see nnl::action::ActionConfig
 * @see nnl::action::IsOfType
 * @see nnl::action::Import
 * @see nnl::action::Export
 */
#pragma once

#include <map>
#include <set>

#include "NNL/common/fixed_type.hpp"
#include "NNL/common/io.hpp"
#include "NNL/game_asset/behavior/action_common.hpp"
#include "NNL/utility/math.hpp"
#include "tsl/ordered_map.h"

namespace nnl {
/**
 * \addtogroup Action
 * @{
 */

/**
 * @copydoc action.hpp
 *
 */
namespace action {
/**
 * \defgroup Action_Main Main
 * @ingroup Action
 * @{
 */

constexpr u16 kNumAnimFuncNSUNI = 0x1C;  ///< The number of functions available for use in
                                         ///< animation nodes in NSUNI

constexpr u16 kNumEffectFuncNSUNI = 0x26;  ///< The number of functions available for use in
                                           ///< effect nodes in NSUNI

constexpr u16 kNumAnimFuncNSLAR = 0x14;  ///< The number of functions available for use in
                                         ///< animation nodes in NSLAR

constexpr u16 kNumEffectFuncNSLAR = 0xC;  ///< The number of functions available for use in
                                          ///< effect nodes in NSLAR
/**
 * @brief Enumeration for different animation functions.
 *
 * This enum defines various functions that can be used in animation nodes to
 * control animation playback.
 *
 * Function used in NSLAR (0x14 in total): 0, 2, 5, 6, 7, 8, a, b, c, e, f,
 * 0x10, 0x13\n\n
 *
 * Functions used in NSUNI (0x1C in total): 0, 2, 6, 7, 8, 9, a, b,
 * c, e, f, 0x10, 0x13, 0x14, 0x16, 0x19, 1a, 1b\n\n
 *
 * Syntax notes:\n
 * [] - represents a list of _argument nodes_ expected by the function\n
 * {} - a single 4 byte node with different layouts.\n
 * a - an argument
 *
 * @see nnl::action::AnimationNode
 */
enum class AnimationFunction : u16 {
  kEnd = 0,   ///< Mark the end of a node sequence; []
  kNext = 1,  ///< Jump to the next specified node; []
  kGoto = 2,  ///< Go to a node; a0 - a relative index of the node to go back or
              ///< forward to: current index + index*4; [{i32}]
  // 3 NULL, causes a crash
  // 4 NULL, causes a crash
  kTransitionToAnimation5 = 5,  ///< Seems to be almost the same as 0x6; [{i16,i16}]
  kTransitionToAnimation6 = 6,  ///< Transition into another animation from
                                ///< the current pose; a0 - animation id, a1 - num
                                ///< of frames the transition takes; [{i16,i16}]
  kEndAnimationPlayback = 7,    ///< This is used after 0xF, 0x6, 0x5; []
  kInitAnimationPlayback = 8,   ///< This is used before 0xF (sometimes 0x6); []
  kSetFlags = 9,                ///< Writes some flags at the offset, used when jumping,
                                ///< getting hit; a0 - offset, a1 flag, a2 flag [{u16,i8,u8}]
  kInitCountdown = 0xA,         ///< Init a countdown;  a0 - time?, a1 ? [{u16,u16}]
  kCheckCountdown = 0xB,        ///< Check if the countdown is 0; []
  kSetBossHitTarget = 0xC,      ///< Something related to a hit circle on bosses; [{i16,i16}]
  kUnkD = 0xD,                  ///< Might call a function; [{i32}]
  kSetAnimSpeed = 0xE,          ///< Changes the animation playback speed; a0 - speed
                                ///<(4096 is the default) [{i16,_}]
  kPlayAnimation = 0xF,         ///< Play an animation; a0 - animation id (from an
                                ///< AnimationContainer);  [{i16,_}]
  kSetAnimStartFrame = 0x10,    ///< Play the animation from a frame; a0 - frame
                                ///< to start playback from;  [{u16,_}]
  kUnk11 = 0x11,                ///< Might call a function; [{u16,_},{u32}]
  kUnk12 = 0x12,                ///< ?; if a0 == 0 animation gets stuck; [{u16,_}]
  kUnk13 = 0x13,                ///< Might call a function; [{unused}]
  // NSUNI ONLY:
  kSetMeshGroupVisib = 0x14,     ///< Change visibility of a mesh group;  a0 - mesh
                                 ///< group, a1 - visibility status; [{u16,u16}]
  kWaitAfter = 0x15,             ///< Pause after executing animation and before going to
                                 ///< the next node;  a0 - time in frames; [{u16,_}]
  kWaitBefore = 0x16,            ///< Pause before executing animation and before going to
                                 ///< the next node; a0 - time; [{u16,_}]
  kEnableTextureSwap = 0x17,     ///< Activate a texture swap; a0 - texture swap id; [{u16,_}]
  kDisableTextureSwap = 0x18,    ///< Disable a texture swap; a0 - texture swap id; [{u16,_}]
  kEnableEffect = 0x19,          ///< Trigger an effect; a0 effect id (e.g. 0xF shakes
                                 ///< the camera);  [{u16,u16},{u16,u16}]
  kUnk1A = 0x1A,                 ///< Might call a function;
                                 ///< [{u16,u16},{u16,u16},{f32},{f32},{f32},{f32}]
  kSetComboTransitFrames = 0x1B  ///< Set combo animation transition frames: a0 - action id ,a1 = -1;
                                 ///< a0 cooldown, a1 time frame to trigger the next attack;
                                 ///< [{i16,i16},{i16,i16},{i16,i16}]
};
/**
 * @brief A union that represents a function or argument node within an animation playback chain.
 *
 * @note When next_main_node is > 1, nodes that immediately follow store arguments for
 * the function and may be reinterpreted as various numeric types.
 *
 * @see nnl::action::AnimationFunction
 * @see nnl::action::ReadArgs
 * @see nnl::action::EffectNode
 * @see nnl::action::Action
 */
union AnimationNode {
  NNL_PACK(struct {
    AnimationFunction function;  ///< What function this node calls.
    u8 next_main_node;           ///< A relative index from this node to the next main
                                 ///< node (also defines the length of the current subchain);
    bool flag;                   ///< This is not used but it's set to true for a
                                 ///< few functions (7, 9, 0xA, 0xB)
  })
  main;  ///< Layout of a main node

  u8 args[4];  ///< Raw memory for storing argument values in argument nodes.
};

static_assert(sizeof(AnimationNode) == 0x4);

/**
 * @brief Enumeration for different effect functions.
 *
 * This enum defines various functions that can be used **during** animation
 * playback that is controlled by **animation nodes**.
 *
 * Functions used in NSLAR (0xC in total): 0, 1, 2, 3, 4, 6, 7, a, b;\n\n
 *
 * Functions used in NSUNI (0x26 in total): 0, 1, 2, 3, 6, 7, a, b, 0x14, 0x17,
 * 0x19, 1a, 1b, 1d, 1e, 1f, 0x20, 0x21, 0x22, 0x23;\n\n
 *
 * Syntax notes:\n
 * [] - represents a list of argument nodes expected by the function\n
 * {} - a single 4 byte node with different layouts\n
 * <{}> - an optional node\n
 *
 * @see nnl::action::EffectNode
 */
enum class EffectFunction : u8 {
  kEnd = 0,                 ///< Mark the end of a node sequence; []
  kSetFlags1 = 1,           ///< Set some flags [<{u16,u16}>, <{i16,_}>]; 0-2 args in
                            ///< NSUNI, 0-3 in NSLAR  [<{u16,i8,u8}>];
  kEnableEffect = 2,        ///< Change the effect that appears from footsteps (?); a0
                            ///< effect id [{u16, i16}];
  kOpacityTransition3 = 3,  ///< ? a0 - type, a1 - num frames, a2 - ? [{u16, i16}, {f32}]
  kOpacityTransition4 = 4,  ///< ? [{u16, i16},{f32}]
  // 5 NULL
  kOpacityTransition6 = 6,  ///< ? [{u16,i16},{i16,i16}]
  kPlaySFX = 7,             ///< Play a sound from a sound bank; a0 - sound id, a1 - ?, a2 - ?
                            ///< [{u16,u8,__},{f32}]
  kPlaySFXRND = 8,          ///< Play one of the 2 sounds from a sound bank; a0, a1 -
                            ///< sound id [{u16, u16},{u8 = 1, u8 = 1,__}]
  // 9 NULL
  kPlayUVAnim = 0xA,          ///< a0 - uv anim id, a1 - u_shift, a2 - v_shift [{u16,i16},{i16,_}]
  kOpacityTransitionB = 0xB,  ///< a0 - unk, a1 - vfx group id, [{u8, u8, u16}, {f32}]
  kUnkC = 0xC,                ///< ?
  kNextD = 0xD,               ///< Does nothing?; []
  kNextE = 0xE,               ///< Does nothing?; []
  kNextF = 0xF,               ///< Does nothing?; []
  kUnk10 = 0x10,              ///< ?
  kUnk11 = 0x11,              ///< ?
  kUnk12 = 0x12,              ///< ?
  kSetFlags13 = 0x13,         ///< ?
  // NSUNI only:
  kEnableEffect14 = 0x14,       ///< a0 - effect id [{u16,i16}]
  kSetFlags15 = 0x15,           ///< ? [{u32},{u16,u16}]
  kSetFlags16 = 0x16,           ///< [{u32},{u16,u16}]
  kUnk17 = 0x17,                ///< ? [{u16,i16},{f32},{f32},{f32},{f32}];
  kUnk18 = 0x18,                ///< ?
  kSlowdown = 0x19,             ///< ?
  kEnableDisplayEffect = 0x1A,  ///< Enable a display effect. The num of args may vary
                                ///< [{u16 - effect id, u16},...];\n [{u16 = 3,u16 = time},
                                ///< {unused}] transition to a white screen;\n [{u16 = 4, u16 =
                                ///< time},{unused}] transition from a white/black screen\n [{u16
                                ///< = 7,??},{}] light camera shake\n [{u16 = 9,__},{}] mid
                                ///< camera shake\n [{u16 = a,__},{}] strong camera shake\n
                                ///< [{u16 = b,__},{}] extreme camera shake\n [{u16 =
                                ///< C,__},{},{f32 = scale},{f32 = scale},{f32 = scale},{u32
                                ///< = duration},{__,_u8 = intensity}] motion blur\n [{u16 = D, u16
                                ///< time},{},{f32 x},{f32 y}] camera shake in the x, y range
  kEnableEffect1B = 0x1B,       ///< a0 effect group, a1 effect id, a2 repeat interval (if 0 - play
                                ///< once) [{u16,__}, {u16, u16}]
  kUnk1C = 0x1C,                ///< ?
  kAnimateCamera = 0x1D,        ///< [{u16 num extra args,u16 - play time num frames},{u16 -
                                ///< transition to default camera,u16 - fps to transition back in
                                ///< start pos},{f32 = x start},{f32 = y start},{f32 = x
                                ///< shift},{f32 = y shift},{f32 pitch},{f32 yaw },{f32
                                ///< roll},{f32 pitch shift},{f32 yaw shift},{f32 roll
                                ///< shift},{f32 fov (extra)},{f32 pan(extra)},{f32 pan
                                ///< shift},{f32}]
  kMoveRootBone = 0x1E,         ///< a0 - mode, a1 num frames, a2, a3, a4 x,y,z
                                ///< [{u16,u16},{f32},{f32},{f32},{f32},{u16,u16},{u16,u16}]
  kAnimateCamera1F = 0x1F,      ///< Almost the same as 0x1D but more arg nodes are
                                ///< used (17 - 19) [...]
  kSetMeshGroupVisib = 0x20,    ///< a0 - group id, a1 - enabled/disabled [{u16, u16}]
  kEnableJutsuEffect = 0x21,    ///< The num of args may vary.
  kSetTextureSwapStat = 0x22,   ///< a0 - swap id, a1 - enabled/disabled [{u16, u16}]
  kSetFlags23 = 0x23,           ///< Set some character flags (?) [{i16,i16},{i16,i16},{i16,i16}]
  kUnk24 = 0x24,                ///? Used in "Scene" action configs [{}, {}]
  kUnk25 = 0x25                 ///?

};
/**
 * @brief A union that represents a function or argument node within an auxiliary effect playback chain.
 *
 * Effect nodes trigger additional visual and sound effects _during_
 * the playback of a main animation chain.
 *
 * @note When next_main_node is > 1, nodes that immediately follow store arguments for
 * the function and may be reinterpreted as various numeric types.
 *
 * @see nnl::action::EffectFunction
 * @see nnl::action::AnimationNode
 * @see nnl::action::Action
 */
union EffectNode {
  NNL_PACK(struct {
    EffectFunction function;  ///< What function this node calls/
    u8 next_main_node;        ///< A relative index from this node to the next main
                              ///< node (also the num of nodes that belong to this
                              ///< function)
    u16 frame_ticks;          ///< At what frame this node is called (relative to the
                              ///< start of the animation node sequence)
  })
  main;

  u8 args[4];  ///< Array for storing argument values (in argument nodes)
};

static_assert(sizeof(EffectNode) == 0x4);

/**
 * @brief Represents an action.
 *
 * An action is a behavior that an entity can perform, such as running, jumping,
 * or attacking. It may consist of multiple animations accompanied by various
 * visual and sound effects.
 *
 * @see nnl::action::ActionConfig
 */
struct Action {
  std::string name = "";                       ///< The name of the action. It does not affect anything.
  std::vector<AnimationNode> animation_nodes;  ///< A series of nodes that configure what animations
                                               ///< are played during the action and how they are
                                               ///< played.

  std::vector<EffectNode> effect_nodes;  ///< A series of nodes that can trigger additional effects
                                         ///< during the animation playback.
};

/**
 * @brief Represents a collection of actions associated with an entity.
 *
 *
 * @note The specific actions and their usage within the games are partially
 * hardcoded.\n
 *
 * @note This object stores only a portion of the data that may be
 * required by a 3D asset to function properly. Its binary representation must
 * be used as part of a larger container that may also include models, animations,
 * and other data.
 *
 * @see nnl::action::Action
 * @see nnl::action::Import
 * @see nnl::asset::Asset3D
 */

using ActionConfig = tsl::ordered_map<action::Id, Action>;

/**
 * @brief Tests if the provided file is an action config.
 *
 * This function takes a file buffer and checks
 * whether it corresponds to the in-game action config format.
 *
 * @param buffer The file buffer to be tested.
 * @return Returns true if the file is identified as an action config
 *
 * @see nnl::action::Import
 */
bool IsOfType(BufferView buffer);

bool IsOfType(const std::filesystem::path& path);

bool IsOfType(Reader& f);

/**
 * @brief Parses a binary file and converts it to ActionConfig.
 *
 * This function takes a binary representation of an action config,
 * parses its contents, and converts them into an `ActionConfig` object for
 * easier access and manipulation.
 *
 * @param buffer The binary file buffer to be processed.
 * @return An `ActionConfig` object representing the converted data.
 *
 * @see nnl::action::IsOfType
 * @see nnl::action::Export
 */
ActionConfig Import(BufferView buffer);

ActionConfig Import(const std::filesystem::path& path);

ActionConfig Import(Reader& f);

/**
 * @brief Converts ActionConfig to a binary file representation.
 *
 * This function takes an `ActionConfig` object and converts it into its binary
 * format.
 *
 * @param action_config The `ActionConfig` object to be converted into the
 * binary format.
 * @return A buffer containing the binary representation of the action
 * configuration.
 */
[[nodiscard]] Buffer Export(const ActionConfig& action_config);

void Export(const ActionConfig& action_config, const std::filesystem::path& path);

void Export(const ActionConfig& action_config, Writer& f);
/** @} */  // Main

/**
 * \defgroup Action_Auxiliary Auxiliary
 * @ingroup Action
 * @{
 */

/**
 * @brief Returns a map of animation IDs and names of all actions in which
 * those animations were used.
 *
 *
 * @param action_config An ActionConfig object containing actions.
 * @return A map where the key is the animation id and the value is a set
 * of action names that use the corresponding animation.
 */
std::map<u16, std::set<std::string>> GetAnimationNames(const ActionConfig& action_config);

/**
 * @brief Extracts arguments from a series of nodes.
 *
 * This function reads arguments from argument nodes of a given main node,
 * taking alignment into account, and stores them in a tuple.
 *
 * For example, given a node chain consisting of [{main}, {u16, u8, u8}, {f32}], 3 arguments can be extracted as
 * follows:
 * @code
 * auto [a0, a1, a2] = Read<u16, u8, f32>(0, nodes);
 * @endcode
 *
 * @tparam Ts Types of the arguments to be extracted.
 * @tparam Node The type of the node (deduced)
 * @param main_node_ind Index of the main node in a vector of nodes from which
 * to extract arguments.
 * @param nodes A vector that stores nodes.
 * @return A tuple containing the extracted arguments.
 *
 */
template <class... Ts, class Node>
inline std::tuple<Ts...> ReadArgs(std::size_t main_node_ind, const std::vector<Node>& nodes) {
  static_assert(std::is_same_v<Node, AnimationNode> || std::is_same_v<Node, EffectNode>, "not a node");
  std::tuple<Ts...> result;
  std::size_t offset = 0;
  std::apply(
      [&](auto&... args) {
        (([&]() {
           static_assert(std::is_trivially_copyable_v<std::remove_reference_t<decltype(args)>>);
           static_assert(sizeof(args) <= sizeof(Node));

           offset = utl::math::RoundNum(offset, sizeof(args));

           auto& main_node = nodes.at(main_node_ind).main;

           std::size_t required_size =
               main_node_ind + 1 + (utl::math::RoundNum(offset + sizeof(args), sizeof(Node)) / sizeof(Node));

           if (required_size > nodes.size())
             NNL_THROW(nnl::RangeError(NNL_SRCTAG("Read exceeds the size of available nodes")));

           if (sizeof(Node) + offset + sizeof(args) > main_node.next_main_node * sizeof(Node))
             NNL_THROW(nnl::RangeError(NNL_SRCTAG("Read exceeds the size of the argument nodes")));

           std::memcpy(&args, reinterpret_cast<const char*>(nodes.data() + main_node_ind + 1) + offset, sizeof(args));

           offset += sizeof(args);
         }()),
         ...);
      },
      result);

  return result;
}
/**
 * @brief Converts arguments to a series of nodes.
 *
 * This function converts arguments to nodes,
 * taking alignment into account. It will expand the provided vector of nodes if
 * there's not enough space. For example, writing to a chain with a single [{main}] node can be done as follows:
 * @code
 * // Initial chain: [{main}]
 * // Resulting chain: [{main}, {u16, u8, u8}, {f32}]
 * Write<u16, u8, f32>(0, nodes, 0x10, 0x1, 1.0f);
 * @endcode
 *
 * If `next_main_node` is not 0, an error is thrown
 * in case the write operation exceeds the existing memory region for arguments.
 *
 * If `next_main_node` is 0, the value is updated to the correct number.
 *
 * @tparam Ts Types of the arguments to be extracted.
 * @tparam Node The type of the node. This can
 * be deduced automatically.
 * @param main_node_ind Index of the main node in a vector of nodes for which to
 * write arguments.
 * @param nodes A vector that stores nodes.
 * @param args A variadic number of values to be converted to nodes.
 *
 */
template <class... Ts, class Node>
inline void WriteArgs(std::size_t main_node_ind, std::vector<Node>& nodes, const Ts&&... args) {
  static_assert(std::is_same_v<Node, AnimationNode> || std::is_same_v<Node, EffectNode>, "not a node");
  std::size_t offset = 0;
  (
      [&] {
        static_assert(std::is_trivially_copyable_v<std::remove_reference_t<decltype(args)>>);

        static_assert(sizeof(args) <= sizeof(Node));

        auto& main_node = nodes.at(main_node_ind).main;

        offset = utl::math::RoundNum(offset, sizeof(args));

        std::size_t required_size =
            main_node_ind + 1 + (utl::math::RoundNum(offset + sizeof(args), sizeof(Node)) / sizeof(Node));

        if (main_node.next_main_node != 0 &&
            sizeof(Node) + offset + sizeof(args) > main_node.next_main_node * sizeof(Node))
          NNL_THROW(nnl::RangeError(NNL_SRCTAG("Write exceeds the size of the argument nodes")));

        if (required_size > nodes.size()) {
          nodes.resize(required_size);
        }

        std::memcpy(reinterpret_cast<char*>(nodes.data() + main_node_ind + 1) + offset, &args, sizeof(args));

        offset += sizeof(args);
      }(),
      ...);

  std::size_t index_next_node = 1 + utl::math::RoundNum(offset, sizeof(Node)) / sizeof(Node);

  auto& main_node = nodes.at(main_node_ind).main;

  if (index_next_node > main_node.next_main_node) main_node.next_main_node = index_next_node;
}
/**
 * @brief Calculates the absolute index of the next main node in a vector.
 *
 * @tparam Node The type of the node (deduced).
 * @param main_node_ind Index of the main node in a vector.
 * @param nodes A vector that stores nodes.
 * @return The absolute index of the next main node in the nodes vector
 *
 */
template <class Node>
inline std::size_t NextNodeInd(std::size_t main_node_ind, const std::vector<Node>& nodes) {
  static_assert(std::is_same_v<Node, AnimationNode> || std::is_same_v<Node, EffectNode>, "not a node");
  const auto& main_node = nodes.at(main_node_ind).main;
  return main_node_ind + main_node.next_main_node;
}
/** @} */  // Aux

namespace raw {
/**
 * \defgroup Action_Raw Raw
 * @ingroup Action
 *
 * The `raw` namespace contains structures that define
 * the unprocessed binary representation of an action config and functions to
 * work with it. This includes low-level data structures that are used to
 * serialize and deserialize information directly from binary files.
 *
 * The intricate representations in the raw namespace are simplified in the
 * main `action` namespace to provide a more user-friendly interface.
 * @{
 */

NNL_PACK(struct RActionCategory {
  u32 offset = 0;
  u32 num_actions = 0;
});

static_assert(sizeof(RActionCategory) == 0x8);

NNL_PACK(struct RHeader { RActionCategory action_categories[kNumActionCategories]; });

static_assert(sizeof(RHeader) == 0x20);

NNL_PACK(struct RAction {
  u16 offset_animation_nodes = 0;
  u16 offset_action_name = 0;
  u16 offset_effect_nodes = 0;
  u16 offset_unknown = 0;  // reserved?
});

static_assert(sizeof(RAction) == 0x8);

NNL_PACK(struct RAnimationNode {
  u16 index_function = 0;
  u8 next_main_node = 1;
  u8 unk_flag = 0;
});

static_assert(sizeof(RAnimationNode) == 0x4);
static_assert(sizeof(RAnimationNode) == sizeof(AnimationNode));

NNL_PACK(struct REffectNode {
  u8 index_function = 0;
  u8 next_main_node = 0;
  u16 time_tick = 0;
});

static_assert(sizeof(REffectNode) == 0x4);
static_assert(sizeof(REffectNode) == sizeof(EffectNode));

struct RActionConfig {
  RHeader header;
  // Maps of offsets (used in the structures) to their data:
  std::map<u32, std::vector<RAction>> actions;
  std::map<u16, std::vector<RAnimationNode>> animation_nodes;
  std::map<u16, std::string> action_names;
  std::map<u16, std::vector<REffectNode>> effect_nodes;
};

ActionConfig Convert(RActionConfig&& raction_config);

RActionConfig Parse(Reader& f);
/** @} */  // Raw
}  // namespace raw
}  // namespace action
/** @} */
}  // namespace nnl
