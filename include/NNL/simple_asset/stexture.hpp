/**
 * @file stexture.hpp
 * @brief Provides data structures for representing essential components of a
 * texture.
 *
 * This file defines structures that facilitate the conversion between common
 * image formats (such as JPG and PNG) and in-game texture formats.
 *
 * @see nnl::STexture
 * @see nnl::texture::Texture
 */
#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "NNL/common/fixed_type.hpp"
#include "NNL/common/io.hpp"
#include "NNL/simple_asset/svalue.hpp"

namespace nnl {

/**
 * \defgroup STexture Simple Texture
 * @ingroup SimpleAssets
 * @copydoc stexture.hpp
 * @{
 */

/**
 * @brief Represents a pixel with red, green, blue, and alpha color channels.
 *
 * @see nnl::STexture
 */
struct SPixel {
  u8 r;  ///< Red channel [0, 255]
  u8 g;  ///< Green channel [0, 255]
  u8 b;  ///< Blue channel [0, 255]
  u8 a;  ///< Alpha channel [0, 255]
  /**
   * @brief Converts the pixel into a single 32-bit ABGR color.
   *
   * @return A 32-bit unsigned integer representing the color.
   */
  operator u32() const { return (a << 24) | (b << 16) | (g << 8) | r; }
};

/**
 * @brief Specifies the filtering methods used for texture sampling.
 *
 * @see nnl::STexture
 * @see https://registry.khronos.org/OpenGL-Refpages/es2.0/xhtml/glTexParameter.xml
 */
enum class STextureFilter {
  kNearest = 0,               ///< Nearest-neighbor filtering.
  kLinear = 1,                ///< Linear filtering.
  kNearestMipmapNearest = 4,  ///< Nearest filtering with mipmap selection.
  kLinearMipmapNearest = 5,   ///< Linear filtering with mipmap selection.
  kNearestMipmapLinear = 6,   ///< Nearest filtering with mipmap selection.
  kLinearMipmapLinear = 7     ///< Linear filtering with mipmap selection.
};

/**
 * @brief Represents a simple texture.
 *
 * This struct defines essential components of a
 * texture. It serves as an intermediary
 * representation for image data, facilitating the conversion between common
 * image formats (such as JPG and PNG) and in-game texture formats.
 *
 * @see nnl::texture::Texture
 * @see nnl::texture::Convert
 * @see nnl::STexture::Import
 * @see nnl::STexture::ExportPNG
 */
struct STexture {
  std::string name;  ///< An optional name for the texture.

  unsigned int width = 0;  ///< Width of the texture in pixels.

  unsigned int height = 0;  ///< Height of the texture in pixels.

  STextureFilter min_filter = STextureFilter::kLinearMipmapNearest;  ///< Texture sampling filter.

  STextureFilter mag_filter = STextureFilter::kLinear;  ///< Texture sampling filter.

  std::vector<SPixel> bitmap;  ///< Pixel data of the texture.

  SValue extras;  ///< Any additional custom data.

  /**
   * @brief Resizes the texture.
   *
   * @param new_width The new width for the texture.
   * @param new_height The new height for the texture.
   */
  void Resize(unsigned int new_width, unsigned int new_height);

  /**
   * @brief Constructs a texture from the specified file path.
   *
   * Loads texture data from common image formats (JPG, PNG, BMP, TGA)
   *
   * @param path The file path to the texture image.
   * @param flip Specifies whether to vertically flip the image during loading.
   */
  [[nodiscard]] static STexture Import(const std::filesystem::path& path, bool flip = false);

  /**
   * @brief Constructs a texture from the specified buffer.
   *
   * Loads texture data from common image formats (JPG, PNG, BMP, TGA)
   *
   * @param buffer The file buffer containing an image.
   * @param flip Specifies whether to vertically flip the image during loading.
   */
  [[nodiscard]] static STexture Import(BufferView buffer, bool flip = false);

  /**
   * @brief Creates a chessboard pattern texture.
   *
   * This static method generates a chessboard pattern texture
   * of the specified width and height.
   *
   * @param width The width of the chessboard texture. It must be a power of 2.
   * @param height The height of the chessboard texture. It must be a power
   * of 2.
   * @return A new STexture object representing the chessboard pattern.
   */
  [[nodiscard]] static STexture GenerateChessPattern(unsigned int width = 128, unsigned int height = 128);

  /**
   * @brief Exports the texture to a PNG file.
   *
   * @param path The file path to save the texture image.
   * @param flip Whether to flip the image vertically during exporting.
   */
  void ExportPNG(const std::filesystem::path& path, bool flip = false) const;

  /**
   * @brief Exports the texture to a PNG file.
   *
   * @param flip Whether to flip the image vertically during exporting.
   * @return A Buffer containing the exported texture data.
   */
  [[nodiscard]] Buffer ExportPNG(bool flip = false) const;

  /**
   * @brief Flips the texture vertically.
   */
  void FlipV();

  /**
   * @brief Aligns the width to the next power of 2.
   *
   * If the texture width is not a power of 2, this method extends it to the
   * next power of 2, placing the original texture on a bigger canvas. This may
   * be useful since the @ref GE supports only power of 2 textures.
   */
  void AlignWidth();

  /**
   * @brief Aligns the height to the next power of 2.
   *
   * If the texture height is not a power of 2, this method extends it to the
   * next power of 2, placing the original texture on a bigger canvas. This may
   * be useful since the @ref GE supports only power of 2 textures.
   */
  void AlignHeight();

  /**
   * @brief Reduces alpha channel precision to the specified number of levels.
   *
   * This may be useful for optimizing textures for CLUT formats.
   *
   * @param levels Number of alpha levels (1-256). Default 256 (no change).
   *               1 makes the image fully opaque.
   */
  void QuantizeAlpha(unsigned int levels = 256);

  /**
   * @brief Checks if the texture contains any transparent pixels.
   *
   * @return True if the texture has transparent pixels.
   */
  bool HasAlpha() const;
};
/** @} */

}  // namespace nnl
