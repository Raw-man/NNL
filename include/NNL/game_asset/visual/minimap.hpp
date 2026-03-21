/**
 * @file minimap.hpp
 * @brief Contains functions and structures for working with minimap configs.
 *
 *
 * @see nnl::minimap::MinimapConfig
 * @see nnl::minimap::IsOfType
 * @see nnl::minimap::Import
 * @see nnl::minimap::Export
 *
 */
#pragma once

#include "NNL/common/fixed_type.hpp"
#include "NNL/common/io.hpp"
#include "NNL/simple_asset/sasset3d.hpp"
namespace nnl {
/**
 * \defgroup Minimap Minimap
 * @ingroup Visual
 * @copydoc minimap.hpp
 * @{
 */
/**
 * @copydoc minimap.hpp
 *
 */
namespace minimap {
/**
 * \defgroup Minimap_Main Main
 * @ingroup Minimap
 * @{
 */
/**
 * @brief Defines settings for a minimap.
 *
 */

struct MinimapConfig {
  f32 anchor_x = 0.0f;         ///< The world-space X anchor, offset by the PSP's half-screen width scaled by the pixel
                               ///< ratio. ((world_x + (240.0f / pixels_per_unit)))
  f32 anchor_z = 0.0f;         ///< The world-space Z anchor, offset by the PSP's half-screen height scaled by the pixel
                               ///< ratio. ((world_z + (136.0f / pixels_per_unit)))
  f32 pixels_per_unit = 0.0f;  ///< Ratio of texture pixels to world units (Texture Width / World Width)
  std::vector<glm::vec3> markers;  ///< Used only in NSUNI, mostly to mark closed areas.
                                   ///< X/Y are _pixel_ coordinates, Z is rotation in degrees.
};

/**
 * @brief Tests if the provided file is a minimap config.
 *
 * This function takes data representing a file and checks
 * whether it corresponds to the in-game minimap config format.
 *
 * @param buffer The data to be tested.
 * @return Returns true if the file is identified as a minimap config;
 *
 * @see nnl::minimap::Import
 */
bool IsOfType(BufferView buffer);

bool IsOfType(const std::filesystem::path& path);

bool IsOfType(Reader& f);

/**
 * @brief Parses a binary file and converts it to a MinimapConfig struct.
 *
 * This function takes a binary representation of a minimap config,
 * parses its contents, and converts them into a `MinimapConfig` struct for easier
 * access and manipulation.
 *
 * @param buffer The binary data to be processed.
 * @return A `MinimapConfig` object representing the converted data.
 *
 * @see nnl::minimap::IsOfType
 * @see nnl::minimap::Export
 */
MinimapConfig Import(BufferView buffer);

MinimapConfig Import(const std::filesystem::path& path);

MinimapConfig Import(Reader& f);

/**
 * @brief Converts a minimap config to a binary file representation.
 *
 * This function takes a `MinimapConfig` object and converts it into a Buffer,
 * which represents the binary format of the minimap config.
 *
 * @param minimap The `MinimapConfig` object to be converted into a binary format.
 * @return A `Buffer` containing the binary representation of the config.
 */
[[nodiscard]] Buffer Export(const MinimapConfig& minimap);

void Export(const MinimapConfig& minimap, const std::filesystem::path& path);

void Export(const MinimapConfig& minimap, Writer& f);
/**
 * @brief Converts a minimap config to a simplified representation.
 *
 * This function converts an in-game minimap config to a list of world-space positions.
 *
 * The first SPosition object in the resulting vector represents the world-space
 * origin aligned with the **top-left** corner of the minimap image.
 * The second SPosition object marks the world-space boundary aligned with the
 * **top-right** corner of the image. The remaining objects (if present)
 * represent various map markers in the world space.
 *
 * @param minimap The input config to be converted.
 *
 * @param texture_width The width of the minimap texture (e.g., 128 or 480).
 *
 * @return A vector of `SPosition` objects.
 *
 * @see nnl::SPosition
 */
std::vector<SPosition> Convert(const MinimapConfig& minimap, unsigned int texture_width);
/**
 * @brief Converts a list of positions to the in-game minimap config format.
 *
 *
 * The first SPosition object is assumed to be the world-space
 * origin aligned with the **top-left** corner of a minimap image.
 * The second SPosition object is assumed to be the world-space boundary aligned with the
 * **top-right** corner of the same image. The remaining objects, if present,
 * are converted to 2D map markers.
 *
 * @param minimap_pos The input positions to be converted.
 *
 * @param texture_width The width of the minimap texture.
 *
 * @return A vector of `SPosition` objects.
 *
 * @note Square Power of Two textures are preferred since the same
 * pixel-to-unit ratio is used for both axes.
 *
 * @see nnl::SPosition
 */
MinimapConfig Convert(const std::vector<SPosition>& minimap_pos, unsigned int texture_width);
/** @} */
namespace raw {
/**
 * \defgroup Minimap_Raw Raw
 * @ingroup Minimap
 * @{
 */

struct RMinimapConfig {
  u32 num_markers = 0;
  u32 reserved_0 = 0;
  u32 reserved_1 = 0;
  u32 reserved_2 = 0;
  std::vector<Vec4<f32>> markers;
};

MinimapConfig Convert(const RMinimapConfig& rminimap);

RMinimapConfig Parse(Reader& f);
/** @} */
}  // namespace raw

}  // namespace minimap
/** @} */
}  // namespace nnl
