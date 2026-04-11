/**
 * @file posd.hpp
 * @brief Contains functions and structures for working with spawn
 * position configs.
 *
 * @see nnl::posd::PositionData
 * @see nnl::posd::IsOfType
 * @see nnl::posd::Import
 * @see nnl::posd::Export
 * @see nnl::posd::Convert
 *
 */
#pragma once

#include <vector>

#include "NNL/common/io.hpp"
#include "NNL/simple_asset/sasset3d.hpp"
#include "NNL/utility/data.hpp"
namespace nnl {
/**
 * \defgroup Position Spawn Position Data (POSD)
 * @ingroup World
 * @copydoc posd.hpp
 * @{
 */
/**
 * @copydoc posd.hpp
 *
 */
namespace posd {
/**
 * \defgroup Position_Main Main
 * @ingroup Position
 * @{
 */
/**
 * @struct Position
 * @brief Represents a spawn position in the world.
 *
 * This struct is used to set spawn positions of objects, NPC's, and mission
 * circles.
 *
 * @see nnl::posd::PositionData
 */
struct Position {
  u32 id = 0;                ///< Unique identifier for the position. The game may look for
                             ///< this id when spawning an entity.
  glm::vec3 position{0.0f};  ///< The 3D coordinates (x, y, z) of the position
  f32 radius = 80.0f;        ///<  May affect the size of a mission circle
  f32 rotation = 0.0f;       ///< Rotation angle around the Y axis in degrees [-180.0, 180.0f]
};
/**
 * @brief A vector of Position structs.
 *
 * This alias represents a collection of Position objects that
 * set spawn positions of various objects in a scene.
 *
 * @see nnl::posd::Import
 */
using PositionData = std::vector<Position>;
/**
 * @brief Converts a spawn position config into a format that is more suitable
 * for further exporting into other formats.
 *
 * @param posd The spawn position config to be converted.
 * @return A vector of spawn positions.
 */
std::vector<SPosition> Convert(const PositionData& posd);
/**
 * @brief Converts a vector of spawn positions to a spawn position config.
 *
 * @param positions A vector of spawn positions to be converted.
 * @return A spawn position config.
 */
PositionData Convert(const std::vector<SPosition>& positions);

/**
 * @brief Tests if the provided file is a spawn position config.
 *
 * This function takes data representing a file and checks
 * whether it corresponds to the in-game spawn position config.
 *
 * @param buffer The data to be tested.
 * @return Returns true if the file is identified as a spawn position config.
 *
 * @see nnl::posd::Import
 */
bool IsOfType(BufferView buffer);

bool IsOfType(const std::filesystem::path& path);

bool IsOfType(Reader& f);

/**
 * @brief Parses a binary file and converts it to PositionData.
 *
 * This function takes a binary representation of a spawn position config,
 * parses its contents, and converts them into a `PositionData` object for easier
 * access and manipulation.
 *
 * @param buffer The binary data to be processed.
 * @return A `PositionData` object representing the converted data.
 *
 * @see nnl::posd::IsOfType
 * @see nnl::posd::Export
 */
PositionData Import(BufferView buffer);

PositionData Import(const std::filesystem::path& path);

PositionData Import(Reader& f);
/**
 * @brief Converts a spawn position config to a binary file representation.
 *
 * This function takes a `PositionData` object and converts it into a Buffer,
 * which represents the binary format of the spawn position config.
 *
 * @param posd The `PositionData` object to be converted into a binary format.
 * @return A `Buffer` containing the binary representation of the config.
 */
[[nodiscard]] Buffer Export(const PositionData& posd);

void Export(const PositionData& posd, const std::filesystem::path& path);

void Export(const PositionData& posd, Writer& f);
/** @} */

namespace raw {

/**
 * \defgroup Position_Raw Raw
 * @ingroup Position
 * @{
 */
constexpr u32 kMagicBytes = utl::data::FourCC("POSD");  ///< Magic bytes

NNL_PACK(struct RHeader {
  u32 magic_bytes = kMagicBytes;
  u32 num_positions = 0;
});

static_assert(sizeof(RHeader) == 0x8);

NNL_PACK(struct RPosition {
  u32 id = 0;
  Vec3<f32> position{0.0f};
  f32 radius = 0.0f;
  i16 rotation = 0;  // fixed point (4 bits - int, 11 - fraction)
  u16 padding = 0;
});

static_assert(sizeof(RPosition) == 0x18);

struct RPositionData {
  RHeader header;
  std::vector<RPosition> positions;
};

PositionData Convert(const RPositionData& posd);

RPositionData Parse(Reader& f);
/** @} */
}  // namespace raw

}  // namespace posd
/** @} */
}  // namespace nnl
