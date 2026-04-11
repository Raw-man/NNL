/**
 * @file fog.hpp
 * @brief Contains functions and structures for working with fog configs
 * used by NSLAR.
 *
 * @see nnl::fog::Fog
 * @see nnl::fog::IsOfType
 * @see nnl::fog::Import
 * @see nnl::fog::Export
 *
 */
#pragma once

#include "NNL/common/fixed_type.hpp"
#include "NNL/common/io.hpp"
#include "NNL/utility/data.hpp"
namespace nnl {
/**
 * \defgroup Fog Fog
 * @ingroup Visual
 * @copydoc fog.hpp
 * @{
 */
/**
 * @copydoc fog.hpp
 *
 */
namespace fog {
/**
 * \defgroup Fog_Main Main
 * @ingroup Fog
 * @{
 */
/**
 * @brief Defines settings for depth-based environmental fog.
 *
 * @note This type of config is used only by NSLAR! While it can be found
 * in NSUNI, it has no functional impact there. @see nnl::render::RenderConfig
 */
struct Fog {
  f32 near_ = 0.0f;           ///< Distance where the fog starts to cover objects
  f32 far_ = 0.0f;            ///< Distance where the fog fully covers objects
  u32 color = 0xFF'00'00'00;  ///< Color of the fog in the ABGR format (alpha is
                              ///< ignored)
};

/**
 * @brief Tests if the provided file is a fog config.
 *
 * This function takes data representing a file and checks
 * whether it corresponds to the in-game fog config format.
 *
 * @param buffer The data to be tested.
 * @return Returns true if the file is identified as a fog config;
 *
 * @see nnl::fog::Import
 */
bool IsOfType(BufferView buffer);

bool IsOfType(const std::filesystem::path& path);

bool IsOfType(Reader& f);

/**
 * @brief Parses a binary file and converts it to a Fog struct.
 *
 * This function takes a binary representation of a fog config,
 * parses its contents, and converts them into a `Fog` struct for easier
 * access and manipulation.
 *
 * @param buffer The binary data to be processed.
 * @return A `Fog` object representing the converted data.
 *
 * @see nnl::fog::IsOfType
 * @see nnl::fog::Export
 */
Fog Import(BufferView buffer);

Fog Import(const std::filesystem::path& path);

Fog Import(Reader& f);

/**
 * @brief Converts a fog config to a binary file representation.
 *
 * This function takes a `Fog` object and converts it into a Buffer,
 * which represents the binary format of the fog config.
 *
 * @param fog The `Fog` object to be converted into a binary format.
 * @return A `Buffer` containing the binary representation of the config.
 */
[[nodiscard]] Buffer Export(const Fog& fog);

void Export(const Fog& fog, const std::filesystem::path& path);

void Export(const Fog& fog, Writer& f);
/** @} */
namespace raw {
/**
 * \defgroup Fog_Raw Raw
 * @ingroup Fog
 * @{
 */
constexpr u32 kMagicBytes = utl::data::FourCC("F0G!");  ///< F0G! in ASCII

NNL_PACK(struct RFog {
  u32 magic_bytes = kMagicBytes;
  f32 near_ = 0.0f;
  f32 far_ = 0.0f;
  u32 color = 0;
});

static_assert(sizeof(RFog) == 0x10);

Fog Convert(const RFog& rfog);

RFog Parse(Reader& f);
/** @} */
}  // namespace raw

}  // namespace fog
/** @} */
}  // namespace nnl
