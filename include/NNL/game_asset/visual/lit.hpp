/**
 * @file lit.hpp
 * @brief Contains structures and functions for working with light
 * source configs.
 *
 * @see nnl::lit::Lit
 * @see nnl::lit::IsOfType
 * @see nnl::lit::Import
 * @see nnl::lit::Export
 * @see nnl::lit::Convert
 *
 */
#pragma once
#include <array>

#include "NNL/common/fixed_type.hpp"
#include "NNL/common/io.hpp"
#include "NNL/simple_asset/sasset3d.hpp"
#include "NNL/utility/data.hpp"

namespace nnl {
/**
 * \defgroup Light Light Sources (LIT)
 * @ingroup Visual
 * @copydoc lit.hpp
 * @{
 */
/**
 * @copydoc lit.hpp
 *
 */
namespace lit {
/**
 * \defgroup Light_Main Main
 * @ingroup Light
 * @{
 */

/**
 * @brief Represents a directional light in the scene.
 * This structure represents a directional light source that affects dynamically
 * lit materials.
 *
 * @see nnl::lit::Lit
 * @see nnl::model::MaterialFeatures::kLit
 */
struct Light {
  bool active = false;                 ///< Indicates if the light is used.
  glm::vec3 direction = {0, 1.0f, 0};  ///< Direction **to** the light source.
  u32 diffuse = 0xFF'FF'FF'FF;         ///< Diffuse color of the light in ABGR format
                                       ///< (alpha is ignored)
  u32 specular = 0xFF'00'00'00;        ///< Specular color of the light in ABGR format
                                       ///< (alpha is ignored)
};
/**
 * @brief Represents a lighting configuration for a scene.
 *
 */
struct Lit {
  std::array<Light, 3> lights;                          ///< Light sources that affect dynamically lit materials.
                                                        ///< @see nnl::model::MaterialFeatures::kLit
  glm::vec3 cel_shadow_light_direction = {0, 1.0f, 0};  ///< Controls the direction of the environmental "light".
                                                        ///< @see nnl::model::MaterialFeatures::kEnvironmentMapping
  u8 character_brightness = 0xFF;                       ///< A multiplier for characters' ambient,
                                                        ///< diffuse, specular colors.
  u32 global_ambient = 0xFF'7F'7F'7F;                   ///< Global ambient light in
                                                        ///< ABGR format (alpha is ignored)
};
/**
 * @brief Converts a light config to a more generic
 * representation that is more suitable for exporting
 * into other formats.
 *
 * @param lit The light config to be converted.
 * @return A vector of lights.
 */
std::vector<nnl::SLight> Convert(const Lit& lit);
/**
 * @brief Converts a vector of lights to a light config.
 *
 * @param slights A vector of lights to be converted.
 * @param ambient The color of the ambient light. If set to (0,0,0), it's
 * computed automatically.
 * @param enable_specular If the specular component should be enabled.
 * @param character_brightness A multiplier that affects materials of
 * characters.
 * @return A light config.
 *
 */
Lit Convert(const std::vector<nnl::SLight>& slights, glm::vec3 ambient = glm::vec3(0), bool enable_specular = true,
            float character_brightness = 1.0f);

/**
 * @brief Tests if the provided file is a light source config.
 *
 * This function takes data representing a file and checks
 * whether it corresponds to the in-game light source config format.
 *
 * @param buffer The data to be tested.
 * @return Returns true if the file is identified as a light config.
 *
 * @see nnl::lit::Import
 */
bool IsOfType(BufferView buffer);

bool IsOfType(const std::filesystem::path& path);

bool IsOfType(Reader& f);

/**
 * @brief Parses a binary file and converts it to a Lit struct.
 *
 * This function takes a binary representation of a light config,
 * parses its contents, and converts them into a `Lit` struct for easier
 * access and manipulation.
 *
 * @param buffer The binary data to be processed.
 * @return A `Lit` object representing the converted data.
 *
 * @see nnl::lit::IsOfType
 * @see nnl::lit::Export
 */
Lit Import(BufferView buffer);

Lit Import(const std::filesystem::path& path);

Lit Import(Reader& f);
/**
 * @brief Converts a light config to a binary file representation.
 *
 * This function takes a `Lit` object and converts it into a Buffer,
 * which represents the binary format of the light config.
 *
 * @param lit The `Lit` object to be converted into a binary format.
 * @return A `Buffer` containing the binary representation of the config.
 */
[[nodiscard]] Buffer Export(const Lit& lit);

void Export(const Lit& lit, const std::filesystem::path& path);

void Export(const Lit& lit, Writer& f);
/** @} */

namespace raw {
/**
 * \defgroup Light_Raw Raw
 * @ingroup Light
 * @{
 */
constexpr u32 kMagicBytes = utl::data::FourCC("LIT!");  ///< Magic bytes

NNL_PACK(struct RLight {
  Vec3<i8> direction{0};
  Vec3<u8> diffuse{0};   // rgb
  Vec3<u8> specular{0};  // rgb
});

static_assert(sizeof(RLight) == 0x9);

NNL_PACK(struct RLit {
  u32 magic_bytes = kMagicBytes;       // 0x0
  u8 active_flags = 0b0'0'0;           // 0x4
  RLight lights[4] = {};               // 0x5 4th light is a placeholder
  Vec3<i8> shadow_light_direction{0};  // 0x29
  u8 character_brightness = 0;         // 0x2C
  Vec3<u8> global_ambient{0};          // 0x2D rgb
  u32 unknown30 = 0xFF'FF'FF'FF;       // 0x30 unused
  u8 unknown34 = 0xFF;                 // 0x34 unused
});

static_assert(sizeof(RLit) == 0x35);

Lit Convert(const RLit& rlit);

RLit Parse(Reader& f);
/** @} */
}  // namespace raw
}  // namespace lit
/** @} */
}  // namespace nnl
