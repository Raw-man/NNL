/**
 * @file texture.hpp
 * @brief Contains structures and functions for working with in-game
 * textures.
 *
 * @see nnl::texture::TextureContainer
 * @see nnl::texture::IsOfType
 * @see nnl::texture::Import
 * @see nnl::texture::Export
 * @see nnl::texture::Convert
 */
#pragma once

#include <math.h>

#include <cstring>
#include <map>
#include <vector>

#include "NNL/common/fixed_type.hpp"
#include "NNL/common/io.hpp"
#include "NNL/simple_asset/stexture.hpp"
#include "NNL/utility/static_vector.hpp"

namespace nnl {
/**
 * \defgroup Texture Texture
 * @ingroup Visual
 * @copydoc texture.hpp
 * @{
 */
/**
 * @copydoc texture.hpp
 *
 */
namespace texture {
/**
 * \defgroup Texture_Main Main
 * @ingroup Texture
 * @{
 */

/**
 * @brief Minimum allowed width and height of a texture in pixels.
 *
 */
constexpr unsigned int kMinDimension = 1;
/**
 * @brief Minimum width of a texture buffer **in bytes** (as expected by
 * the @ref GE). The actual pixel width can be smaller than this.
 *
 */
constexpr unsigned int kMinByteBufferWidth = 16;
/**
 * @brief Maximum allowed width and height of a texture in pixels.
 *
 * The @ref GE supports a maximum width of only 512 pixels. However,
 * emulators can handle textures up to 1024 pixels in width and height.
 */
constexpr unsigned int kMaxDimension = 1024;

/**
 * @brief The maximum number of mipmap levels that can be used.
 *
 */
constexpr std::size_t kMaxMipMapLvl = 7;

/**
 * @brief The maximum number of texture levels.
 *
 */
constexpr std::size_t kMaxTextureLvl = 1 + kMaxMipMapLvl;

/**
 * @brief Enumeration of texture formats.
 *
 * This enum defines the various texture formats that can be used to store
 * texture data. The values correspond to what is expected by the @ref GE.
 * Technically, there are a few more formats available, but they are not
 * used by the games and are rare in general.
 *
 * @see nnl::texture::Texture
 */
enum class TextureFormat : u16 {
  kCLUT8 = 5,     ///< 8 bit indices that refer to a color lookup table with up to
                  ///< 256 colors.
  kCLUT4 = 4,     ///< 4 bit indices that refer to a color lookup table with up to
                  ///< 16 colors.
  kRGBA8888 = 3,  ///< 32-bit color format with 8 bits per channel. No CLUT.
  kRGBA4444 = 2,  ///< 16-bit color format with 4 bits per channel. No CLUT.
  kRGBA5551 = 1,  ///< 16-bit color format. No CLUT.
  kRGB565 = 0     ///< 16-bit color format. No CLUT.
};

/**
 * @brief Enumeration of color formats used in Color Lookup Tables (CLUT).
 *
 *
 * @see nnl::texture::Texture
 */
enum class ClutFormat : u8 {
  kRGBA8888 = 3,  ///< 32-bit color format. 8 bits per channel.
  kRGBA4444 = 2,  ///< 16-bit color format. 4 bits per channel.
  kRGBA5551 = 1,  ///< 16-bit color format. 1 bit for the alpha channel.
  kRGB565 = 0     ///< 16-bit color format. No alpha channel.
};

/**
 * @brief Enumeration of texture filtering methods.
 *
 * This enum defines various texture filtering methods that can be
 * applied when rendering textures larger or smaller than their original size.
 *
 * The filtering methods include options that can be used for both magnification
 * and minification, as well as mipmapping.
 *
 * These values map directly to what the @ref GE expects.
 *
 * @see nnl::texture::Texture
 * @see https://registry.khronos.org/OpenGL-Refpages/es2.0/xhtml/glTexParameter.xml
 */
enum class TextureFilter : u8 {
  kNearest = 0,  ///< Nearest-neighbor filtering.
  kLinear = 1,   ///< Linear filtering.
  // minifying only:
  kNearestMipmapNearest = 4,  ///< Nearest filtering with mipmap selection.
  kLinearMipmapNearest = 5,   ///< Linear filtering with mipmap selection.
  kNearestMipmapLinear = 6,   ///< Nearest filtering with mipmap selection.
  kLinearMipmapLinear = 7     ///< Linear filtering with mipmap selection.
};
/**
 * @brief Structure representing image data of a texture.
 *
 * It can be used to represent both the main texture level and its mipmaps.
 *
 * @note For proper functionality, both texture width and height
 * must be powers of 2, especially when mipmapping is used. The PSP's @ref GE
 * can only load textures in powers of 2. If the height is not a power of 2
 * (a practice often used in UI textures to save space), it's rounded up and
 * extra garbage data is loaded. If width is not a power of 2 (which is the
 * case for few invalid textures), it's rounded down and part of the texture is
 * discarded.
 *
 * @see nnl::texture::Texture
 */
struct TextureData {
  u16 width = 0;         ///< Width of the texture in pixels. It **must** be a power of 2.
  u16 height = 0;        ///< Height of the texture in pixels. It generally should be a power
                         ///< of 2. However, it may not always be the case for UI textures.
  u16 buffer_width = 0;  ///< Width of the bitmap_buffer **in pixels**. It must be greater than or
                         ///< equal to the width. The buffer width **in bytes** must be a multiple of 16.
                         ///< For example, in a CLUT4 texture, this value must be at least 32 **pixels**.
  Buffer bitmap_buffer;  ///< Raw bitmap data. Its interpretation depends on the texture_format.
};
/**
 * @brief Structure representing a texture.
 *
 * This struct encapsulates all the necessary information for a texture. It can
 * store both the main texture level and its mipmaps.
 *
 * @see nnl::texture::TextureContainer
 */
struct Texture {
  Buffer clut_buffer;                                            ///< Color Look Up Table. It's used when texture_format
                                                                 ///< is set to one of the CLUT formats.
  utl::static_vector<TextureData, kMaxTextureLvl> texture_data;  ///< Stores various levels of the texture. Index 0
                                                                 ///< contains the full-resolution base texture, with
                                                                 ///< subsequent indices storing mipmaps. Limited to 8
                                                                 ///< levels in total.
  TextureFormat texture_format = TextureFormat::kCLUT8;          ///< Format of the texture data.
  bool swizzled = true;                                          ///< Indicates whether the texture data is reorganized
                                                                 ///< for more efficient use by the GE
  TextureFilter min_filter = TextureFilter::kLinearMipmapNearest;  ///< Minification filter method for texture sampling.
  TextureFilter mag_filter = TextureFilter::kLinear;               ///< Magnification filter method for texture
                                                                   ///< sampling: kLinear or kNearest.
  ClutFormat clut_format = ClutFormat::kRGBA8888;                  ///< Format of the CLUT data.
};
/**
 * @brief Type alias for a container of textures.
 *
 * @note This struct usually stores only a portion of the data required by an
 * asset to function properly. Its binary representation must be used as part
 * of various larger containers that may also include UI configs, text, models
 * and other data.
 *
 * @see nnl::asset::Asset
 * @see nnl::asset::Asset3D
 * @see nnl::asset::BitmapText
 * @see nnl::model::Model
 */
using TextureContainer = std::vector<Texture>;

/**
 * @brief Structure to hold parameters for texture conversion.
 *
 * This structure encapsulates various parameters that control the behavior of
 * texture conversion from a simplified representation to the in-game format.
 *
 * @see nnl::texture::Convert
 */
struct ConvertParam {
  TextureFormat texture_format = TextureFormat::kCLUT8;  ///< The format the texture will be converted to.
  ClutFormat clut_format = ClutFormat::kRGBA8888;        ///< The format the color lookup table will use
                                                         ///< (if texture_format uses it).
  bool clut_dither = true;                               ///< Whether dithering should be applied to the
                                                         ///< texture if a CLUT format is used. This may give
                                                         ///< an illusion of a greater color variety.
  unsigned int max_mipmap_lvl = kMaxMipMapLvl;           ///< The maximum number of mipmap levels to generate.
  bool gen_small_mipmap = false;                         ///< Whether to generate mipmaps with width
                                                         ///< that is smaller than the minimum buffer width.
  unsigned int max_width_height = kMaxDimension / 2;     ///< Maximum dimensions for the texture; larger
                                                         ///< textures will be resized.
  bool swizzle = true;  ///< Whether to reorganize the texture data for more efficient use. @see
                        ///< nnl::texture::SwizzleFromMem
};

/**
 * @brief Converts a container of textures to a simplified representation.
 *
 * This function takes a `TextureContainer` containing multiple in-game textures
 * and converts each texture into a simplified representation.
 *
 * @param texture_container The input container holding the textures to be
 * converted.
 *
 * @param mipmaps A flag indicating whether mipmaps should be extracted as well.
 *
 * @return A vector of `STexture` objects representing the converted
 * textures.
 *
 * @see nnl::STexture
 */
std::vector<STexture> Convert(const TextureContainer& texture_container, bool mipmaps = false);

/**
 * @brief Converts a vector of simplified textures into the in-game format.
 *
 * This function takes a vector of `STexture` images and converts them
 * into a `TextureContainer`, applying the specified conversion parameters.
 *
 * @param images A vector of `STexture` objects representing the textures to be converted.
 * @param texture_params A `ConvertParam` object specifying the conversion settings for each texture.
 *
 * @return A `TextureContainer` object containing the converted textures.
 *
 * @note Use `std::move` when passing an existing object.
 */
TextureContainer Convert(std::vector<STexture>&& images, const ConvertParam& texture_params = {});

TextureContainer Convert(std::vector<STexture>&& images, const std::vector<ConvertParam>& texture_params);

/**
 * @brief Tests if the provided file is a texture container.
 *
 * This function takes a file buffer and checks
 * whether it corresponds to the in-game texture container format.
 *
 * @param buffer The file buffer to be tested.
 * @return Returns true if the file is identified as a texture container.
 *
 * @see nnl::texture::Import
 */
bool IsOfType(BufferView buffer);

bool IsOfType(const std::filesystem::path& path);

bool IsOfType(Reader& f);

/**
 * @brief Parses a binary file and converts it to a structured texture
 * container.
 *
 * This function takes a binary representation of a texture container, parses
 * its contents, and converts them into a `TextureContainer` vector for easier
 * access and manipulation.
 *
 * @param buffer The binary file to be processed.
 * @return A `TextureContainer` object representing the converted data.
 *
 * @see nnl::texture::IsOfType
 * @see nnl::texture::Export
 */
TextureContainer Import(BufferView buffer);

TextureContainer Import(const std::filesystem::path& path);

TextureContainer Import(Reader& f);

/**
 * @brief Converts a texture container to its binary file representation.
 *
 * This function takes a `TextureContainer` object and converts it into
 * the binary format.
 *
 * @param texture_container The `TextureContainer` object to be converted.
 * @return A buffer containing the binary representation of the texture
 * container.
 */
[[nodiscard]] Buffer Export(const TextureContainer& texture_container);

void Export(const TextureContainer& texture_container, const std::filesystem::path& path);

void Export(const TextureContainer& texture_container, Writer& f);
/** @} */  // Main

/**
 * \defgroup Texture_Auxiliary Auxiliary
 * @ingroup Texture
 * @{
 */

/**
 * @brief Converts a texture from a simplified representation to the in-game
 * format.
 *
 * This function takes a simple texture and converts it to the in-game format
 * using the provided settings.
 *
 * @param image The input texture to be converted.
 * @param tex_param A `ConvertParam` structure containing the conversion
 * settings.
 *
 * @return A `Texture` object representing the converted texture.
 *
 * @note Use `std::move` when passing an existing object.
 *
 * @see nnl::texture::TextureContainer
 */
Texture Convert(STexture&& image, const ConvertParam& tex_param = {});
/**
 * @brief Converts a texture from the in-game format to a simplified
 * representation.
 *
 * This function takes an in-game texture in various formats and converts it
 * into a simplified representation.
 *
 * @param texture The input texture to be converted.
 * @param level The texture level to convert. The default value is 0, which
 * corresponds to the base level. Higher values correspond to mipmaps.
 *
 * @return A `STexture` object representing the converted texture.
 *
 * @see nnl::STexture
 */
STexture Convert(const Texture& texture, unsigned int level = 0);

/**
 * @brief Converts an image buffer from RGB565 format to RGBA8888 format.
 *
 * @param bitmap565 A vector containing the image data in RGB565 format.
 *
 * @return A vector containing the converted image data in RGBA8888 format.

 */
Buffer ConvertRGB565ToRGBA8888(BufferView bitmap565);
/**
 * @brief Converts an image buffer from RGBA5551 format to RGBA8888 format.
 *
 * @param bitmap5551 A vector containing the image data in RGBA5551 format.
 *
 * @return A vector containing the converted image data in RGBA8888 format.

 */
Buffer ConvertRGBA5551ToRGBA8888(BufferView bitmap5551);
/**
 * @brief Converts an image buffer from RGBA4444 format to RGBA8888 format.
 *
 * @param bitmap4444 A vector containing the image data in RGBA4444 format.
 *
 * @return A vector containing the converted image data in RGBA8888 format.

 */
Buffer ConvertRGBA4444ToRGBA8888(BufferView bitmap4444);
/**
 * @brief Converts an image buffer from RGBA8888 format to RGBA4444 format.
 *
 * @param bitmap8888 A vector containing the image data in RGBA8888 format.
 *
 * @return A vector containing the converted image data in RGBA4444 format.

 */
Buffer ConvertRGBA8888ToRGBA4444(BufferView bitmap8888);
/**
 * @brief Converts an image buffer from RGBA8888 format to RGBA5551 format.
 *
 * @param bitmap8888 A vector containing the image data in RGBA8888 format.
 *
 * @return A vector containing the converted image data in RGBA5551 format.

 */
Buffer ConvertRGBA8888ToRGBA5551(BufferView bitmap8888);
/**
 * @brief Converts an image buffer from RGBA8888 format to RGB565 format.
 *
 * @param bitmap8888 A vector containing the image data in RGBA8888 format.
 *
 * @return A vector containing the converted image data in RGB565 format.

 */
Buffer ConvertRGBA8888ToRGB565(BufferView bitmap8888);
/**
 * @brief Unswizzles texture data.
 *
 * This function takes texture data that was reordered for more efficient use by
 * the @ref GE and converts it back to a linear format.
 *
 * @param texptr A vector containing the swizzled texture data.

 * @param bufw The buffer width of the texture in pixels
 *
 * @param height The height of the texture in pixels
 *
 * @param bytes_per_pixel The number of bytes used to represent each pixel. 0 for less than a byte.
 *
 * @return A vector containing the unswizzled texture data in linear format.
 */
Buffer UnswizzleFromMem(BufferView texptr, u32 bufw, u32 height, u32 bytes_per_pixel);
/**
 * @brief Swizzles texture data.
 *
 * This function takes linear texture data and reorders it to make it more
 * adapted to the way the @ref GE reads it. This is only applicable to textures
 * with buffer width bigger than 16 **bytes**.
 *
 *
 * @param texptr A vector containing texture data.

 * @param bufw The buffer width of the texture **in pixels**
 *
 * @param height The height of the texture **in pixels**
 *
 * @param bytes_per_pixel The number of bytes used to represent each pixel. 0 for less than a byte.
 *
 * @return A vector containing swizzled texture data.
 */
Buffer SwizzleFromMem(Buffer& texptr, u32 bufw, u32 height, u32 bytes_per_pixel);
/**
 * @brief Computes a quick hash for a texture.
 *
 * It can generate a **part** of a hash that is used by
 * @ref PPSSPP for texture caching and replacement.
 *
 *
 * @param data The data to be hashed.
 * @return A 32-bit unsigned integer representing the computed hash value of
 *         the data.
 */
u32 QuickTexHash(BufferView data);
/**
 * @brief Converts a 4-bit indexed texture to an 8-bit indexed texture.
 *
 *
 * @param clut4 A vector containing the texture data in 4-bit indexed format.
 * @param width Texture width
 * @return A vector containing the converted data in 8-bit indexed format.
 */
Buffer ConvertIndexed4To8(BufferView clut4, u32 width);
/**
 * @brief Converts an 8-bit indexed texture to a 4-bit indexed texture.
 *
 *
 * @param clut8 A vector containing the texture data in 8-bit indexed format.
 * @param width Texture width
 * @return A vector containing the converted data in 4-bit indexed format.
 */
Buffer ConvertIndexed8To4(BufferView clut8, u32 width);
/**
 * @brief Creates a color palette using the median cut algorithm.
 *
 *
 * @param source_image The source texture from which to generate the color
 * palette.
 *
 * @param num_colors The maximum number of colors in the generated palette.
 *
 * @return The generated color palette.
 *
 */
std::vector<SPixel> GeneratePaletteMedian(const STexture& source_image, std::size_t num_colors = 256);
/**
 * @brief Creates a color palette by extracting the first N unique colors from the image.
 *
 *
 * @param source_image The source texture from which to generate the color
 * palette.
 *
 * @param num_colors The maximum number of colors in the generated palette.
 *
 * @return The generated color palette.
 *
 * @see nnl::texture::GeneratePaletteMedian
 *
 */
std::vector<SPixel> GeneratePaletteNaive(const STexture& source_image, std::size_t num_colors = 256);
/**
 * @brief Applies a color palette to the image.
 *
 * This function converts the image to an indexed color format using the
 * specified palette. Replaces each pixel's color with the closest matching
 * color from the palette.
 *
 * @param image The source texture to which the palette will be applied.
 * @param palette A vector of `SPixel` representing the color palette
 *                to be applied to the texture.
 * @param dither A flag indicating whether dithering should be used
 *                during the palette application. It can help reduce
 *                color banding artifacts.
 *
 * @return A vector containing an 8-bit indexed image data.
 */
std::vector<u8> ApplyPalette(const STexture& image, const std::vector<SPixel>& palette, bool dither = true);

/**
 * @brief Aligns the buffer width to a desired buffer width.
 *
 * This function takes an image buffer and adjusts its width to ensure it's
 * aligned to a minimum of 16 bytes, which is a requirement for the @ref GE.
 *
 * @param image_buffer A vector containing the original image data.
 * @param width The width of the image in pixels.
 * @param height The height of the image in pixels.
 * @param bpp The number of bytes per pixel. 0 for less than a byte.
 * @param buffer_width The desired width of the buffer in pixels.
 * @return A vector containing the aligned buffer data.
 *
 * @see nnl::texture::DealignBufferWidth
 */
Buffer AlignBufferWidth(BufferView image_buffer, unsigned int width, unsigned int height, unsigned int bpp,
                        unsigned int buffer_width);
/**
 * @brief Removes padding from an aligned image buffer.
 *
 * This function takes an image buffer that has been aligned to a certain buffer
 * width and removes the extra padding.
 *
 * @param image_buffer A vector containing the aligned image buffer.
 * @param width The actual width of the image in pixels.
 * @param height The height of the image in pixels.
 * @param bpp The number of bytes per pixel. 0 for less than a byte.
 * @param buffer_width The width of the buffer in pixels.
 * @return A vector containing the de-aligned buffer data.
 *
 * @see nnl::texture::AlignBufferWidthTo16
 */
Buffer DealignBufferWidth(BufferView image_buffer, unsigned int width, unsigned int height, unsigned int bpp,
                          unsigned int buffer_width);
/**
 * @brief Converts an 8-bit indexed image buffer to an RGBA8888 buffer.
 *
 * This function takes an 8-bit indexed image and applies the provided RGBA8888
 * color palette to convert it into a standard RGBA8888 format. Each byte in
 * the indexed image is replaced with its corresponding color from the palette,
 * resulting in a simple RGBA8888 image.
 *
 * @param indexed_image A vector containing the 8-bit indexed image data.
 * @param palette A vector containing the RGBA8888 color palette.
 *
 * @return A vector containing the converted image data in RGBA8888 format.
 */
Buffer DeindexClut8ToRGBA8888(BufferView indexed_image, BufferView palette);

/**
 * @brief Generates conversion parameters from a given texture.
 *
 * This function attempts to create a `ConvertParam` object that would result in
 * a similar in-game texture when converting from a simplified representation
 * back to the in-game format.
 *
 * @param texture The source texture from which to generate conversion
 * parameters.
 * @return A `ConvertParam` object.
 *
 * @see nnl::texture::Convert
 */
ConvertParam GenerateConvertParam(const Texture& texture);
/**
 * @brief Generates conversion parameters from a given texture container.
 *
 * This function attempts to create a vector of `ConvertParam` objects that
 * would result in similar in-game textures when converting from a simplified
 * texture representation back to the in-game format.
 *
 * @param texture_container The source container holding the textures from which
 *                         to generate conversion parameters.
 * @return A vector of `ConvertParam` objects.
 *
 * @see nnl::texture::Convert
 */
std::vector<ConvertParam> GenerateConvertParam(const TextureContainer& texture_container);

/** @} */

namespace raw {

/**
 * \defgroup Texture_Raw Raw
 * @ingroup Texture
 *  The `raw` namespace contains structures that define
 * the binary representation of texture data and functions to work with it. This
 * includes low-level data structures that are used to serialize and deserialize
 * texture information directly from binary files.
 *
 * The intricate representations in the raw namespace are simplified in the
 * main `texture` namespace to provide a more user-friendly interface.
 *
 * @{
 */

/**
 * @brief Magic bytes used to identify the texture format.
 *
 * This constant represents a unique identifier that texture containers start
 * with.
 */
constexpr u32 kMagicBytes = 0x88'88'00'01;

constexpr u32 kColorEntriesInBlock = 16;

NNL_PACK(struct RHeader {
  u32 magic_bytes = kMagicBytes;  // 0x0
  u32 offset_textures = 0;        // 0x4
  u32 offset_texture_data = 0;    // 0x8
  u32 offset_clut_offsets = 0;    // 0xC
  u16 num_textures = 0;           // 0x10
  u16 num_texture_data = 0;       // 0x12
  u16 num_clut_offsets = 0;       // 0x14
  u16 unknown = 0;                // 0x16
});

static_assert(sizeof(RHeader) == 0x18, "");

NNL_PACK(struct RTexture {
  u16 texture_format = 0;           // 0x0
  u16 index_texture_data = 0;       // 0x2
  u8 num_texture_data_mipmaps = 0;  // 0x4
  u8 swizzled = false;              // 0x5
  u8 filter_minifying = 0;          // 0x6
  u8 filter_magnifying = 0;         // 0x7
  u16 index_clut = 0;               // 0x8
  u8 clut_load_size = 0;            // 0xA how many blocks of 16 colors to load
  u8 clut_format = 0;               // 0xB
});

static_assert(sizeof(RTexture) == 0xC, "");

NNL_PACK(struct RTextureData {
  u32 offset_bitmap = 0;   // 0x0
  u16 width = 0;           // 0x4
  u16 height = 0;          // 0x6
  u16 buffer_width = 0;    // 0x8
  u16 height_rounded = 0;  // 0xA same as height or rounded to 8 (filled dynamically)
  u32 size = 0;            // 0xC
  u32 address = 0;         // 0x10 filled dynamically
});

static_assert(sizeof(RTextureData) == 0x14, "");

struct RTextureContainer {
  RHeader header;
  std::vector<RTexture> textures;
  std::vector<RTextureData> texture_data;
  std::vector<u32> clut_offsets;
  // Maps of offsets (used in the structures above) to their data:
  std::map<u32, Buffer> bitmaps;
  std::map<u32, Buffer> cluts;
};

TextureContainer Convert(RTextureContainer&& rtexture_container);

RTextureContainer Parse(Reader& f);
/** @} */
}  // namespace raw

}  // namespace texture
/** @} */
}  // namespace nnl
