/**
 * @file shadow_collision.hpp
 * @brief Contains structures and functions for working with shadow
 * "collisions" - geometry onto which fake planar or blob shadows are
 * cast.
 *
 * @see nnl::shadow_collision::Collision
 * @see nnl::shadow_collision::IsOfType
 * @see nnl::shadow_collision::Import
 * @see nnl::shadow_collision::Export
 * @see nnl::shadow_collision::Convert
 */
#pragma once

#include "NNL/common/fixed_type.hpp"
#include "NNL/common/io.hpp"
#include "NNL/simple_asset/smodel.hpp"
#include "NNL/utility/array3d.hpp"
namespace nnl {
/**
 * \defgroup ShadowCollision Shadow Collision
 * @ingroup Interaction
 * @copydoc shadow_collision.hpp
 * @{
 */
/**
 * @copydoc shadow_collision.hpp
 *
 */
namespace shadow_collision {
/**
 * \defgroup Shadow_Main Main
 * @ingroup ShadowCollision
 * @{
 */
/**
 * @brief Struct defining a single triangle for projecting shadows onto it.
 *
 */
struct Triangle {
  std::array<u16, 3> indices_vertices{0};  ///< Indices of the triangle's vertices. @see
                                           ///< nnl::shadow_collision::Collision::vertices
  u16 shadow_features = 0;                 ///< Not used in NSLAR, in @ref NUC2 this can disable shadows
                                           ///< (1) and possibly more...
};
/**
 * @brief Struct representing a set of triangles to be tested for shadow
 * projection.
 *
 * The CollisionTest struct holds the indices of the triangles that are
 * relevant for shadow projection at the player's current position.
 *
 * @see nnl::shadow_collision::Collision
 */
struct CollisionTest {
  std::vector<u16> triangle_indices;
};
/**
 * @brief Struct that contains all necessary information for performing shadow
 * projection.
 *
 * The Collision struct contains parameters and data structures necessary for
 * projection of fake shadows.
 *
 * @note This type of "collision" works only in NSLAR (it's also used in
 * @ref NUC2)\n\n
 *
 * @note This struct stores only a portion of the data that may be
 * required by a 3D asset to function properly. Its binary representation must
 * be used as part of a larger container that should also include models and
 * possibly other data.
 *
 *
 * @see nnl::asset::Asset
 * @see nnl::asset::Asset3D
 * @see nnl::collision::Collision
 * @see nnl::model::Model
 */
struct Collision {
  u16 shift_value = 1;                         ///< The exponent of 2 that determines how the entire
                                               ///< collision space is divided into smaller parts. E.g,
                                               ///< the player's coordinates are shifted by this value
                                               ///< to get a relevant entry in the coordinate_map.
  glm::vec3 origin_point{0.0f};                ///< Represents the origin point in 3D space. All coordinates in
                                               ///< the coordinate_map are assumed to be positive offsets
                                               ///< relative to it.
  std::vector<glm::vec3> vertices;             ///< The vertices that are used to define the triangles
                                               ///< involved in shadow projection.
  std::vector<Triangle> triangles;             ///< List of triangles in the collision environment.
  utl::Array3D<CollisionTest> coordinate_map;  ///< A 3D array that maps coordinates that the player can
                                               ///< end up in to the corresponding collision tests.
                                               ///< ((current_pos - origin_point)>>(shift_value+5))
};
/**
 * @brief Converts collision data from the in-game format to a simplified
 * model representation.
 *
 * @param collision The in-game shadow collision format to be converted.
 * @return The converted model.
 */
SModel Convert(const Collision& collision);
/**
 * @brief Converts a simplified representation of a model to the in-game
 * shadow collision format that is used for shadow projection.
 *
 * @param smodel The source model to be converted.
 * @param auto_cull Automatically remove triangles that do not belong to the
 * ground.
 * @param shift The exponent of 2 that determines how the whole collision space
 * is divided. Larger values can reduce the size of data but make collision
 * testing slower.
 * @return The converted collision.
 *
 * @note Use `std::move` when passing an existing object.
 */
Collision Convert(SModel&& smodel, bool auto_cull = true, u32 shift = 1);

/**
 * @brief Tests if the provided file is a shadow collision.
 *
 * This function takes a file buffer and checks
 * whether it corresponds to the in-game shadow collision format.
 *
 * @param buffer The data to be tested.
 * @return Returns true if the file is identified as a shadow collision;
 *
 * @see nnl::shadow_collision::Import
 */
bool IsOfType(BufferView buffer);

bool IsOfType(const std::filesystem::path& path);

bool IsOfType(Reader& f);

/**
 * @brief Parses a binary file and converts it to a structured collision.
 *
 * This function takes a binary representation of a shadow collision config,
 * parses its contents, and converts them into a `Collision` struct for easier
 * access and manipulation.
 *
 * @param buffer The binary data to be processed.
 * @return A `Collision` object representing the converted data.
 *
 * @see nnl::shadow_collision::IsOfType
 * @see nnl::shadow_collision::Export
 */
Collision Import(BufferView buffer);

Collision Import(const std::filesystem::path& path);

Collision Import(Reader& f);
/**
 * @brief Converts a shadow collision config to a binary file representation.
 *
 * This function takes a `Collision` object and converts it into a Buffer,
 * which represents the binary format of the collision.
 *
 * @param collision The `Collision` object to be converted into a binary format.
 * @return A `Buffer` containing the binary representation of the collision.
 */
[[nodiscard]] Buffer Export(const Collision& collision);

void Export(const Collision& collision, const std::filesystem::path& path);

void Export(const Collision& collision, Writer& f);
/** @} */

namespace raw {
/**
 * \defgroup Shadow_Raw Raw
 * @ingroup ShadowCollision
 * @{
 */
NNL_PACK(struct RHeader {
  Vec4<f32> origin_point{0, 0, 0, 1.0f};  // 0x0
  Vec3<u32> bbox_dimension{0};            // width, height, depth, shifted by shift value
  u32 offset_coordinate_map = 0;          // 0x1C
  u32 offset_vertex_table = 0;            // 0x20
  u32 offset_triangles = 0;               // 0x24
  u32 offset_collision_tests = 0;
  u16 shift_value = 0;  //+5 is always added to it
  u16 unknown = 0;
});

static_assert(sizeof(RHeader) == 0x30);

NNL_PACK(struct RTriangle {
  u16 index_vertex_0 = 0;   // 0x0
  u16 index_vertex_1 = 0;   // 0x2
  u16 index_vertex_2 = 0;   // 0x4
  u16 shadow_features = 0;  // 0x6  unused in NSLAR, in NUC2 this can disable shadow (1) and possibly more
  u32 calculation = 0;      // 0x8
});

static_assert(sizeof(RTriangle) == 0xC);

NNL_PACK(struct RCollisionTest {
  u32 offset_triangle_indices = 0;  // 0x0
  u16 num_triangle_indices = 0;     // 0x4
  u16 padding = 0;                  // 0x6
});

static_assert(sizeof(RCollisionTest) == 0x8);

struct RCollision {
  RHeader header;
  std::vector<Vec4<f32>> vertices;
  std::vector<RTriangle> triangles;
  std::vector<RCollisionTest> collision_tests;
  std::vector<u16> indices_triangles;  // indices to faces to test
  std::vector<u16> coordinate_map;
};

RCollision Parse(Reader& f);

Collision Convert(const RCollision& rcollision);
/** @} */
}  // namespace raw

}  // namespace shadow_collision
/** @} */
}  // namespace nnl
