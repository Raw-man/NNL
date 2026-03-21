/**
 * @file colbox.hpp
 * @brief Provides structures and functions for working with collision boxes of
 * entities.
 *
 * @see nnl::colbox::ColBoxConfig
 * @see nnl::colbox::IsOfType
 * @see nnl::colbox::Import
 * @see nnl::colbox::Export
 */

#pragma once

#include <glm/glm.hpp>
#include <map>

#include "NNL/common/fixed_type.hpp"
#include "NNL/common/io.hpp"
#include "NNL/game_asset/behavior/action_common.hpp"
#include "NNL/game_asset/visual/model_common.hpp"
#include "tsl/ordered_map.h"
namespace nnl {
/**
 * \defgroup Colbox Collision box
 * @ingroup Interaction
 * @copydoc colbox.hpp
 * @{
 */

/**
 * @copydoc colbox.hpp
 *
 */
namespace colbox {
/**
 * \defgroup Colbox_Main Main
 * @ingroup Colbox
 * @{
 */
/**
 * @brief Represents a collision box.
 *
 * This struct allows you to configure collision boxes that can be used in
 * interactions with the environment, other entities, and damage handling.
 *
 * @see nnl::colbox::ColBoxConfig
 */
struct CollisionBox14 {
  model::BoneTarget bone_target = 0;  ///< Specifies the bone to which this
                                      ///< box will be attached. @see nnl::model::HumanoidRigTarget
  u16 unknown2 = 0;                   ///< unknown: equals to 1 in NSLAR (in damage, attack boxes)
  glm::vec3 translation{0.0f};        ///< Specifies the position of the box
                                      ///< relative to the bone.
  f32 size = 4.0f;                    ///< Specifies the size of the box.
};
/**
 * @brief Represents a collision box.
 *
 * This struct allows you to configure collision boxes that can be used to
 * detect collisions with other entities.
 *
 * @note Used only in NSLAR!
 *
 * @see nnl::colbox::ColBoxConfig
 */
struct CollisionBox18 {
  model::BoneTarget bone_target = 0;  ///< Specifies the bone to which this
                                      ///< box will be attached. @see nnl::model::HumanoidRigTarget
  u16 unknown2 = 0;                   ///< unknown
  glm::vec3 translation{0.0f};        ///< Specifies the position of the box
                                      ///< relative to the bone.

  f32 size = 4.0f;       ///< Specifies the size of the box.
  f32 unknown14 = 0.0f;  ///< Unknown
};
/**
 * @brief Represents a collision box in a hitbox config.
 *
 * @see nnl::colbox::ColBoxConfig
 * @see nnl::colbox::HitboxConfig
 */
struct CollisionBox20 {
  model::BoneTarget bone_target = 0;  ///< Specifies the bone to which this
                                      ///< box will be attached. @see nnl::model::HumanoidRigTarget
  u16 distance_attack = 0;            ///< Sets some flags (e.g., if a secondary box is created).
                                      ///< Equals 5 or 1 in NSUNI.
  glm::vec3 translation{0.0f};        ///< Specifies the position of the box
                                      ///< relative to the bone.
  glm::vec3 translation_2{0.0f};      ///< Specifies the position of the second
                                      ///< box (if distance attack is set)
  f32 size = 0;                       ///< Specifies the size of the box.
};

/**
 * @brief Represents a hitbox config.
 *
 * @see nnl::colbox::ColBoxConfig
 * @see nnl::action::ActionConfig
 */
struct HitboxConfig {
  u16 dmg_start_frame = 0;               ///< The frame at which damage starts (relative to the action).
  u16 dmg_end_frame = 0;                 ///< The frame at which damage ends.
  u16 effect_id = 0;                     ///< The ID of the effect played when the attack hits another entity.
                                         ///< This may affect the damage it receives.
  u8 unknownC = 1;                       ///< Unknown, maybe an ID: affects health dmg, in most cases
                                         ///< it goes incrementally
  u8 unknownD = 0;                       ///< Unknown: not used, usually equals to 0 (not in
                                         ///< the monkey boss from NSLAR)
  u16 unknownE = 0;                      ///< Unknown
  std::vector<CollisionBox20> colboxes;  ///< Configures parts that can deal damage.
};

/**
 * @brief Represents the configuration for various collision boxes of a
 * character.
 *
 * This struct contains various configurations for collisions with the environment,
 * other entities, and configurations for both receiving and dealing damage.
 *
 * In NSUNI only attack and damage collisions (hitboxes/hurtboxes)
 * can be changed with this configuration. Other types of collision boxes seem
 * to be unaffected by it. However, everything works correctly in NSLAR.\n
 * @note This struct stores only a portion of the data that may be
 * required by a 3D asset to function properly. Its binary representation must
 * be used as part of a larger container that must also include meshes,
 * animations, an action config and other data.
 *
 * @see nnl::asset::Asset
 * @see nnl::asset::Asset3D
 * @see nnl::model::Model
 * @see nnl::action::ActionConfig
 */
struct ColBoxConfig {
  std::vector<CollisionBox14> col_enviroment;  ///< Configures collisions with the static environment. @see
                                               ///< nnl::collision::Collision @note Functions only in NSLAR!
  std::vector<CollisionBox14> col_unknown;     ///< Unknown collision configuration.
  std::vector<CollisionBox18> col_entities;    ///< Configures collisions with other entities
                                               ///< @note Functions only in NSLAR!
  std::vector<CollisionBox14> col_damage;      ///< Configures parts that can receive damage (hurtboxes).
  tsl::ordered_map<action::Id, std::vector<HitboxConfig>>
      col_attack;  ///< Configures parts that can deal damage (hitboxes).
};

/**
 * @brief Tests if the provided file is a collision box config.
 *
 * @param buffer The file to be tested.
 * @return Returns true if the file is identified as a collision box config;
 *
 * @see nnl::colbox::Import
 */
bool IsOfType(BufferView buffer);

bool IsOfType(const std::filesystem::path& path);

bool IsOfType(Reader& f);

/**
 * @brief Parses a binary file and converts it to a structured
 * CollisionBoxConfig.
 *
 * @param buffer The binary file to be processed.
 * @return A `CollisionBoxConfig` object representing the converted data.
 *
 * @see nnl::colbox::IsOfType
 * @see nnl::colbox::Export
 */
ColBoxConfig Import(BufferView buffer);

ColBoxConfig Import(const std::filesystem::path& path);

ColBoxConfig Import(Reader& f);

/**
 * @brief Converts a CollisionBoxConfig to a binary file representation.
 *
 * @param colbox_config The config to be converted into a binary format.
 * @return A Buffer containing the binary representation of the model.
 */
[[nodiscard]] Buffer Export(const ColBoxConfig& colbox_config);

void Export(const ColBoxConfig& colbox_config, const std::filesystem::path& path);

void Export(const ColBoxConfig& colbox_config, Writer& f);
/** @} */
namespace raw {
/**
 * \defgroup Colbox_Raw Raw
 * @ingroup Colbox
 * @{
 */

/**
 * @brief Magic bytes used to identify the collision box format.
 *
 * This constant represents a unique identifier that collision box configs start with.
 */
constexpr u32 kMagicBytes = 0xFF'FF'00'01;

NNL_PACK(struct RHeader {
  u32 magic_bytes = kMagicBytes;
  u32 offset_colbox_enviroment = sizeof(RHeader);  // 0x4
  u32 offset_colbox_entities = sizeof(RHeader);    // 0x8
  u32 offset_colbox_dmg = sizeof(RHeader);         // 0xC
  u32 offset_colbox_unknown = sizeof(RHeader);     // 0x10 not used; no refernces in the code
  u32 offset_colbox_attack = sizeof(RHeader);      // 0x14 RHitboxHeader

  u32 offset_unknown = 0;         // 0x18
  u32 offset_reserved_0 = 0;      // 0x1C unused
  u32 offset_reserved_1 = 0;      // 0x20
  u32 offset_reserved_2 = 0;      // 0x24
  u32 offset_reserved_3 = 0;      // 0x28
  u16 num_colbox_attack = 0;      // 0x2C
  u16 num_unknown = 0;            // 0x2E always 0
  u16 num_colbox_enviroment = 0;  // 0x30
  u16 num_colbox_entities = 0;    // 0x32
  u16 num_colbox_dmg = 0;         // 0x34
  u16 num_colbox_unknown = 0;     // 0x36 equals to 1 in NSLAR, not used
  u16 num_reserved_0 = 0;
  u16 num_reserved_1 = 0;
  u16 num_reserved_2 = 0;
  u16 num_reserved_3 = 0;
});

static_assert(sizeof(RHeader) == 0x40, "");

NNL_PACK(struct RCollisionBox14 {
  u16 bone_role = 0;            // 0x0
  u16 unknown2 = 0;             // 0x2
  Vec3<f32> translation{0.0f};  // 0x4

  f32 size = 4.0f;  // 0x10
});

static_assert(sizeof(RCollisionBox14) == 0x14, "");

NNL_PACK(struct RCollisionBox18 {
  u16 bone_role = 0;            // 0x0
  u16 unknown2 = 0;             // 0x2
  Vec3<f32> translation{0.0f};  // 0x4
  f32 size = 4.0f;              // 0x10
  f32 unknown14 = 0.0f;         // 0x14
});

static_assert(sizeof(RCollisionBox18) == 0x18, "");

NNL_PACK(struct RHitboxHeader {
  u32 offset_hitboxes = 0;  // 0x0
  u16 num_hitboxes = 0;     // 0x4
  u16 action_id = 0;        // 0x6 The upper 2 bits - action category, the rest (0x3FFF)
                            // - action index.
});

static_assert(sizeof(RHitboxHeader) == 0x8, "");

NNL_PACK(struct RHitbox {
  u32 offset_colboxes = 0;  // 0x0
  u16 num_colboxes = 0;     // 0x4 RColBox20
  u16 dmg_start = 0;        // 0x6
  u16 dmg_end = 0;          // 0x8
  u16 effect_id = 0;        // 0xA
  u8 unknownC = 1;          // 0xC
  u8 unknownD = 0;          // 0xD
  u16 unknownE = 0;         // 0xE
});

static_assert(sizeof(RHitbox) == 0x10, "");

NNL_PACK(struct RCollisionBox20 {
  u16 bone_role = 0;              // 0x0
  u16 distance_attack = 0;        // 0x2
  Vec3<f32> translation{0.0f};    // 0x4
  Vec3<f32> translation_2{0.0f};  // 0x10

  f32 size = 0;  // 0x1C
});

static_assert(sizeof(RCollisionBox20) == 0x20, "");

struct RCollisionBoxConfig {
  RHeader header;
  std::vector<RCollisionBox14> col_enviroment;
  std::vector<RCollisionBox14> colbox_unknown;
  // some unknown structures may be here (see 0x2E in RHeader)
  std::vector<RCollisionBox18> colbox_entities;
  std::vector<RCollisionBox14> colbox_damage;
  std::vector<RHitboxHeader> colbox_attack;

  // Maps of offsets (used in the structures above) to their data:
  std::map<u32, std::vector<RHitbox>> hitboxes;                 // see RHitboxHeader
  std::map<u32, std::vector<RCollisionBox20>> hitbox_colboxes;  // see RHitbox
};

RCollisionBoxConfig Parse(Reader& f);

ColBoxConfig Convert(const RCollisionBoxConfig& rhitbox_config);
/** @} */
}  // namespace raw
}  // namespace colbox
/** @} */
}  // namespace nnl
