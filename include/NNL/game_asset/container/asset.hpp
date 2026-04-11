/**
 * @file asset.hpp
 * @brief Provides structures and functions to manage containers of
 * interrelated asset parts.
 *
 * @see nnl::asset::Asset
 * @see nnl::asset::IsOfType
 * @see nnl::asset::ImportView
 * @see nnl::asset::Import
 * @see nnl::asset::Export
 */
#pragma once

#include <map>
#include <vector>

#include "NNL/common/fixed_type.hpp"
#include "NNL/common/io.hpp"
namespace nnl {

/**
 * \defgroup Asset Asset
 * @ingroup Containers
 * @copydoc asset.hpp
 *
 * @{
 */

/**
 * @copydoc asset.hpp
 *
 */
namespace asset {
/**
 * \defgroup Asset_Main Main
 * @ingroup Asset
 * @{
 */

/**
 * @brief A container for related parts of a complete asset.
 *
 * This container typically holds closely related
 * components of a complete asset.
 *
 * For instance, it can store parts of a 3D object, such as meshes, textures,
 * and animations. In other instances, it can hold components of a text archive,
 * including a bitmap font and textual data. Each individual component is
 * typically associated with a specific numeric key, which depends on the
 * category of the container.
 *
 * @see nnl::asset::Import
 * @see nnl::asset::Category
 * @see nnl::asset::Categorize
 * @see nnl::asset::AssetView
 */
using Asset = std::map<u32, Buffer>;
/**
 * @brief A non-owning view of an asset container.
 *
 * This type is almost identical to Asset and represents the same container but allows for access to its
 * entries via _references_ to the source Buffer. It's most suitable when no modifications to the data are
 * expected.
 *
 * @see nnl::asset::Asset
 * @see nnl::asset::ImportView
 */
using AssetView = std::map<u32, BufferView>;

/**
 * @brief Tests if the provided file is an asset container.
 *
 * This function takes data representing a file and checks
 * whether it corresponds to the in-game asset container format.
 *
 * @param buffer The data to be tested.
 * @return true if the file is identified as an asset container
 *
 * @see nnl::asset::Import
 * @see nnl::asset::ImportView
 */
bool IsOfType(BufferView buffer);

bool IsOfType(const std::filesystem::path& path);

bool IsOfType(Reader& f);

/**
 * @brief Parses a binary file and converts it into an Asset object.
 *
 * This function takes a binary representation of an asset container,
 * parses its contents, and converts them into an Asset object for easier
 * access and modification.
 *
 * @param buffer The binary data to be processed.
 * @return An `Asset` object consisting of numeric keys and file buffers.
 *
 * @see nnl::asset::IsOfType
 * @see nnl::asset::ImportView
 * @see nnl::asset::Export
 */
Asset Import(BufferView buffer);

Asset Import(const std::filesystem::path& path);

Asset Import(Reader& f);
/**
 * @brief Parses a binary file and converts it into an AssetView object.
 *
 * AssetView does not store copies of
 * file buffers but stores _references_ to the original data.
 *
 * @param buffer The binary data to be processed.
 * @return An `AssetView` object representing the converted data.
 *
 * @see nnl::action::IsOfType
 * @see nnl::asset::Asset
 * @see nnl::asset::AssetView
 */
AssetView ImportView(BufferView buffer);
/**
 * @brief Converts an asset container to a binary file representation.
 *
 * This function takes an Asset container and converts it into a Buffer,
 * which represents the binary format of the container.
 *
 * @param asset_container The `Asset` object to be converted.
 * @return A `Buffer` containing the binary representation of the container.
 */
[[nodiscard]] Buffer Export(const Asset& asset_container);

void Export(const Asset& asset_container, const std::filesystem::path& path);

void Export(const Asset& asset_container, Writer& f);

[[nodiscard]] Buffer Export(const AssetView& asset_container);

void Export(const AssetView& asset_container, const std::filesystem::path& path);

void Export(const AssetView& asset_container, Writer& f);

/** @} */

/**
 * \defgroup Asset_Auxiliary Auxiliary
 * @ingroup Asset
 *
 * @{
 */

/**
 * @brief Classification for asset container layouts.
 *
 * Defines a container's category based on the entry types expected
 * at specific keys and the combined asset they represent.
 *
 * @see nnl::asset::Asset
 * @see nnl::asset::Categorize
 */
enum Category : u32 {
  kUnknown = 0,                                      ///< Unknown
  kAsset3DModel = 0b10000001,                        ///< Contains kModel and possibly other parts
                                                     ///< @see nnl::asset::Asset3D
                                                     ///< @see nnl::format::FileFormat
  kAsset3DAnim = 0b10000010,                         ///< Contains kAnimationContainer and other animation types
                                                     ///< for another kAsset3DModel
  kAsset3DAction = 0b10000100,                       ///< Contains only kActionConfig
  kAsset3DEffect = 0b10001000,                       ///< Appears only in NSLAR, contains kActionConfig,
                                                     ///< kCollectionSpline, kTextureContainer2
  kSoundBank = 0b1'00000000,                         ///< A PHD/PBD pair (header + ADPCM data)
                                                     ///< @see nnl::asset::SoundBank
  kBitmapTextFont = 0b101'0'00000000,                ///< Consists of kTextureContainer and kAdvanceWidth
                                                     ///< Used in NSLAR. @see nnl::asset::BitmapText
  kBitmapTextFull = 0b110'0'00000000,                ///< Consists of kTextureContainer, glyph spacing, kText
  kUIConfigs = 0b1'000'0'00000000,                   ///< Consists of multiple kUIConfig's
  kUIConfigTextureContainer = 0b1'0'000'0'00000000,  ///< Consists of pairs of kUIConfig and kTextureContainer
  kPlaceholder = 0x80000000                          ///< No entries
};

/**
 * @brief Constants for accessing entries in an asset container that
 * stores a 3D object.
 *
 * @note The presence of certain entries may vary depending on the specific
 * asset and its usage context.
 *
 * @see nnl::asset::Category
 * @see nnl::asset::Asset
 * @see nnl::format::FileFormat
 */
namespace Asset3D {

constexpr u32 kModel = 0;                  ///< @see nnl::model::Model
constexpr u32 kTextureContainer = 1;       ///< @see nnl::texture::TextureContainer
constexpr u32 kAnimationContainer = 2;     ///< @see nnl::animation::AnimationContainer
constexpr u32 kActionConfig = 3;           ///< @see nnl::action::ActionConfig
constexpr u32 kColboxConfig = 4;           ///< @see nnl::colbox::ColBoxConfig
constexpr u32 kCollectionSpline = 5;       ///< A kCollection of (possibly) splines, found in
                                           ///< few effects in NSLAR
                                           ///< @see nnl::asset::Category::kAsset3DEffect
constexpr u32 kTextureContainer2 = 6;      ///< A regular kTextureContainer. Found in few effects in NSLAR
                                           ///< @see nnl::asset::Category::kAsset3DEffect
constexpr u32 kCollision = 7;              ///< @see nnl::collision::Collision
constexpr u32 kShadowCollision = 8;        ///< @see nnl::shadow_collision::Collision
constexpr u32 kVisanimationContainer = 9;  ///< @see nnl::visanimation::AnimationContainer

}  // namespace Asset3D
/**
 * @brief Constants for accessing entries in an asset container that
 * stores a PHD/PBD sound bank, a proprietary format developed by Sony.
 * More details can be found in the official PSP SDK docs.
 *
 * @see nnl::asset::Category
 * @see nnl::asset::Asset
 * @see nnl::format::FileFormat
 */
namespace SoundBank {

constexpr u32 kPHD = 0;  ///< The PHD header that contains metadata linking attributes for
                         ///< all sounds in the bank. @see nnl::phd::IsOfType
constexpr u32 kPBD = 1;  ///< The raw ADPCM waveform data for _all_ sounds.
                         ///< @see nnl::adpcm::Decode

}  // namespace SoundBank
/**
 * @brief Constants for accessing entries in an asset container that
 * stores a bitmap text archive.
 *
 * @note This type of organization is found in NSUNI. In NSLAR, **different
 * containers** are used.
 *
 * @see nnl::asset::Category
 * @see nnl::asset::Asset
 * @see nnl::format::FileFormat
 */
namespace BitmapText {

constexpr u32 kTextureContainer = 0;  ///< @see nnl::texture::TextureContainer
constexpr u32 kAdvanceWidth = 1;      ///< A plain u8 array specifying the advance width for each character of kText
constexpr u32 kText = 2;              ///< @see nnl::text::Text

}  // namespace BitmapText

/**
 * @brief Determines the category of an asset by analyzing its contents.
 * @param asset The asset to examine.
 * @return The asset's category, or kUnknown if unrecognized.
 *
 * @see nnl::asset::Category
 */
Category Categorize(const Asset& asset);

/**
 * @copydoc Categorize
 */
Category Categorize(const AssetView& asset);

/** @} */

namespace raw {
NNL_PACK(struct RFileRecord {
  u32 offset = 0;
  u32 size = 0;
});

static_assert(sizeof(RFileRecord) == 0x8);

template <typename TData>
struct RAsset {
  std::vector<RFileRecord> file_records;
  // Maps of offsets (used in the structures above) to their data:
  std::multimap<u32, TData> file_buffers;  // offsets may repeat if empty buffers present
};

RAsset<Buffer> Parse(Reader& f);

RAsset<BufferView> ParseView(BufferView buffer);

Asset Convert(RAsset<Buffer>&& rasset);

AssetView Convert(RAsset<BufferView>&& rasset);

}  // namespace raw

}  // namespace asset
/** @} */
}  // namespace nnl
