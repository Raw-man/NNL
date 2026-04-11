/**
 * @file format.hpp
 * @brief Contains the enumeration of various file formats found in the
 * games and functions to detect them.
 *
 * @see nnl::format::FileFormat
 * @see nnl::format::Detect
 *
 */
#pragma once
#include "NNL/common/io.hpp"

namespace nnl {
/**
 * \defgroup FileFormat File Format
 * @ingroup GameAssets
 * @copydoc format.hpp
 * @{
 */
/**
 * @copydoc format.hpp
 *
 */
namespace format {

/**
 * @brief Enumeration for file formats _found_ in the games.
 *
 * @note This list is **non-exhaustive** and represents currently known
 * signatures. Undocumented formats exist within the archives.
 *
 * @see nnl::format::Detect
 * @see nnl::format::DetectAll
 */
enum FileFormat {

  kUnknown,                ///< An unknown type
  kPGD,                    ///< Protected Game Data: an encrypted wrapper that
                           ///< contains a primary data archive (a .BIN file in NSUNI/NSLAR).
  kDig,                    ///< A primary asset archive (a .BIN file in NSUNI/NSLAR). @see nnl::dig::Dig
  kDigEntry,               ///< An entry in a primary archive (a nested archive itself). @see nnl::dig_entry::DigEntry
  kCollection,             ///< A container that stores related assets. @see nnl::collection::Collection
  kAssetContainer,         ///< A container that stores related parts of an asset. @see nnl::asset::Asset
  kModel,                  ///< @see nnl::model::Model
  kTextureContainer,       ///< @see nnl::texture::TextureContainer
  kAnimationContainer,     ///< @see nnl::animation::AnimationContainer
  kActionConfig,           ///< @see nnl::action::ActionConfig
  kColboxConfig,           ///< @see nnl::colbox::ColBoxConfig
  kVisanimationContainer,  ///< @see nnl::visanimation::AnimationContainer
  kCollision,              ///< @see nnl::collision::Collision
  kShadowCollision,        ///< @see nnl::shadow_collision::Collision
  kText,                   ///< @see nnl::text::Text
  kATRAC3,                 ///< An AT3 file that uses a proprietary audio codec by Sony.
  kFog,                    ///< @see nnl::fog::Fog
  kPositionData,           ///< @see nnl::posd::PositionData
  kLit,                    ///< @see nnl::lit::Lit
  kRenderConfig,           ///< @see nnl::render::RenderConfig
  kPHD,                    ///< @see nnl::phd::IsOfType
  kUIConfig,               ///< @see nnl::ui::IsOfType
  kMinimapConfig,          ///< @see nnl::minimap::MinimapConfig
  kPNG,                    ///< A PNG image
  kCCSF,      ///< An asset format developed by CyberConnect2. It's used for the finishing blow cutscenes in NSUNI. @see
              ///< https://github.com/MiguelQueiroz010/CCSF-Asset-Explorer/
  kELF,       ///< An executable file
  kPSPELF,    ///< An encrypted executable file
  kPSF,       ///< PARAM.SFO, metadata
  kPlainText  ///< A plain text file (UTF-8 encoded)
};

/**
 * @brief Detects the file format of the provided buffer.
 *
 * This function analyzes the provided data buffer and returns the
 * first detected file format.
 *
 * @param buffer A file buffer containing data to be tested.
 * @return FileFormat The first matching signature.
 *
 * @see nnl::format::DetectAll
 */
FileFormat Detect(BufferView buffer);

FileFormat Detect(const std::filesystem::path& path);

FileFormat Detect(Reader& f);
/**
 * @brief Detects all possible file formats of the provided buffer.
 *
 * This function analyzes the provided data buffer and returns all
 * detected file formats.
 *
 * @param buffer A file buffer containing data to be tested.
 * @return A vector of matching signatures.
 *
 * @see nnl::format::Detect
 */
std::vector<FileFormat> DetectAll(BufferView buffer);

std::vector<FileFormat> DetectAll(const std::filesystem::path& path);

std::vector<FileFormat> DetectAll(Reader& f);
/** @} */
}  // namespace format
}  // namespace nnl
