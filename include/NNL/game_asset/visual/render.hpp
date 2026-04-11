/**
 * @file render.hpp
 * @brief Contains functions for working with render configs used by
 * NSUNI.
 *
 * @see nnl::render::RenderConfig
 * @see nnl::render::IsOfType
 * @see nnl::render::Import
 * @see nnl::render::Export
 *
 */
#pragma once

#include "NNL/common/fixed_type.hpp"
#include "NNL/common/io.hpp"
namespace nnl {
/**
 * \defgroup Render Render Config
 * @ingroup Visual
 *
 * @copydoc render.hpp
 * @{
 */

/**
 * @copydoc render.hpp
 *
 */
namespace render {
/**
 * \defgroup Render_Main Main
 * @ingroup Render
 * @{
 */
/**
 * @brief Represents settings for rendering.
 *
 * @note This type of config is used only in NSUNI! @see nnl::fog::Fog
 */
struct RenderConfig {
  u8 bloom_translucency = 0x7F;             ///< Controls the visibility of the bloom effect.
                                            ///< 0 means the effect is the strongest.
  f32 mipmap_bias = 0.1f;                   ///< An offset value that is added to the LoD
                                            ///< calculation of a texture (aka OFFL)
  f32 mipmap_slope = 0.002f;                ///< Another value that is used to calculate LoD (aka TSLOPE, see the
                                            ///< official GE docs for more details)
  f32 fog_near = 500.0f;                    ///< Sets the distance where the fog starts to cover _spawned_
                                            ///< entities (characters, crates). It has no effect on map objects.
                                            ///< The value may be negative.
  f32 fog_draw_distance_far = 1200.0f;      ///< Sets the rendering distance and the distance where the
                                            ///< fog fully covers objects. A value of 100.0 is added to it
                                            ///< by the game engine.
  u32 fog_color = 0xFF'00'00'00;            ///< Color of the fog in the ABGR format
                                            ///< (alpha is ignored)
  u8 distance_transition_translucency = 0;  ///< Controls whether objects located at the edge of the rendering
                                            ///< distance disappear immediately or gradually become transparent.
                                            ///< A value of 0 (fully opaque) or 255 means the effect won't be
                                            ///< noticeable. Set it to 0 for the maximum rendering distance.
};

/**
 * @brief Tests if the provided file is a render config.
 *
 * This function takes data representing a file and checks
 * whether it corresponds to the in-game render config format.
 *
 * @param buffer The data to be tested.
 * @return Returns true if the file is identified as a render config;
 *
 * @see nnl::render::Import
 */
bool IsOfType(BufferView buffer);

bool IsOfType(const std::filesystem::path& path);

bool IsOfType(Reader& f);

/**
 * @brief Parses a binary file and converts it to a RenderConfig struct.
 *
 * This function takes a binary representation of a render config,
 * parses its contents, and converts them into a `RenderConfig` struct for easier
 * access and manipulation.
 *
 * @param buffer The binary data to be processed.
 * @return A `RenderConfig` object representing the converted data.
 *
 * @see nnl::render::IsOfType
 * @see nnl::render::Export
 */
RenderConfig Import(BufferView buffer);

RenderConfig Import(const std::filesystem::path& path);

RenderConfig Import(Reader& f);
/**
 * @brief Converts a render config to a binary file representation.
 *
 * This function takes a `RenderConfig` object and converts it into a Buffer,
 * which represents the binary format of the render config.
 *
 * @param render_config The `RenderConfig` object to be converted.
 * @return A `Buffer` containing the binary representation of the config.
 */
[[nodiscard]] Buffer Export(const RenderConfig& render_config);

void Export(const RenderConfig& render_config, const std::filesystem::path& path);

void Export(const RenderConfig& render_config, Writer& f);
/** @} */

namespace raw {
/**
 * \defgroup Render_Raw Raw
 * @ingroup Render
 * @{
 */
constexpr u32 kMagicBytes = 1;  ///< The magic bytes

NNL_PACK(struct RRenderConfig {
  u32 magic_bytes = kMagicBytes;             // 0x0 not very reliable
  u32 unknown4 = 0;                          // 0x4
  u32 unknown8 = 0;                          // 0x8
  u32 unknownC = 0;                          // 0xC
  u32 enable_buffer_effects = 1;             // 0x10 bloom+distance effect: always active!
  u32 bloom_translucency = 0;                // 0x14
  f32 mipmap_bias = 0.0f;                    // 0x18
  f32 mipmap_slope = 0.0f;                   // 0x1C
  f32 draw_distance_near = 0.0f;             // 0x20
  f32 draw_distance_far = 0.0f;              // 0x24
  u32 distance_fog_red = 0;                  // 0x28
  u32 distance_fog_green = 0;                // 0x2C
  u32 distance_fog_blue = 0;                 // 0x30
  u32 distance_transition_translucency = 0;  // 0x34
  u32 padding0 = 0;                          // 0x38
  u32 padding1 = 0;                          // 0x3C
});

static_assert(sizeof(RRenderConfig) == 0x40, "");

RenderConfig Convert(const RRenderConfig& rdistance);

RRenderConfig Parse(Reader& f);
/** @} */
}  // namespace raw

}  // namespace render
/** @} */
}  // namespace nnl
