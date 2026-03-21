/**
 * @file collision.hpp
 * @brief Contains structures and functions for working with static
 * collisions.
 *
 * @see nnl::collision::Collision
 * @see nnl::collision::IsOfType
 * @see nnl::collision::Import
 * @see nnl::collision::Export
 * @see nnl::collision::Convert
 */

#pragma once

#include "NNL/common/enum_flag.hpp"
#include "NNL/common/fixed_type.hpp"
#include "NNL/common/io.hpp"
#include "NNL/simple_asset/smodel.hpp"
#include "NNL/utility/array3d.hpp"
namespace nnl {
/**
 * \defgroup Collision Collision
 * @ingroup Interaction
 * @copydoc collision.hpp
 * @{
 */
/**
 * @copydoc collision.hpp
 *
 */
namespace collision {
/**
 * \defgroup Collision_Main Main
 * @ingroup Collision
 * @{
 */

/**
 * @brief Enum representing different settings for surfaces.
 *
 * The surface features determine how an entity collides with a surface and
 * what sounds and effects are used when it does.
 *
 * @note This enum reflects the original game's binary format. The first 4 bits
 * represent mutually exclusive surface types, while higher bits
 * are additive behavior flags.
 */
enum class NNL_FLAG_ENUM SurfaceFeatures : u16 {
  kNone = 0,              ///< Default, same as kDirt
  kDirt = 0b0000'0001,    ///< Dirt
  kDirt2 = 0b0000'0010,   ///< Another type of dirt
  kSand = 0b0000'0011,    ///< Sand
  kStone = 0b0000'0100,   ///< Stone or gravel
  kGrass = 0b0000'0101,   ///< Grass
  kWater = 0b0000'0110,   ///< Water
  kWood = 0b0000'0111,    ///< Wood
  kStone2 = 0b0000'1000,  ///< Another type of stone
  kMetal = 0b0000'1001,   ///< Metal
  // flags
  kShadow = 0b0001'0000,              ///< Darkens characters that step on the surface.
                                      ///< @note This flag works properly only in NSLAR.
  kPassThrowables = 0b010'0000'0000,  ///< Allows the camera and throwable
                                      ///< objects to pass through the surface.
  kPassCamera = 0b100'0000'0000       ///< Allows the camera to pass through the surface.
};

NNL_DEFINE_ENUM_OPERATORS(SurfaceFeatures)
/**
 * @brief Enum representing different push behaviors for entities.
 *
 * This enum defines various settings that determine how entities
 * interact with collision surfaces when they get too close to them.
 *
 * @see nnl::colbox::ColBoxConfig
 */
enum class NNL_FLAG_ENUM PushFeatures : u16 {
  kNone = 0,           ///< Default
  kPushBack = 0b01,    ///< This flag should be used for meshes that act as walls. When an
                       ///< entity is too close to the object, it will be pushed back to
                       ///< prevent overlap.
  kPushIfStuck = 0b10  ///< This flag should be used for meshes that act as ceilings.
};

NNL_DEFINE_ENUM_OPERATORS(PushFeatures)
/**
 * @brief Struct defining a single triangle used in collision detection.
 *
 * @see nnl::collision::CollisionTest
 * @see nnl::collision::Collision::normals
 * @see nnl::collision::Collision::vertices
 */
struct Triangle {
  std::array<u16, 3> indices_vertices{0};  ///< Indices of the triangle's vertices.
  u16 index_normal = 0;                    ///< Index of the triangle's normal.
  u16 index_vertex_min = 0;                ///< Index of the triangle's bounding minimum coordinate (min x, min
                                           ///< y, min z)
  u16 index_vertex_max = 0;                ///< Index of the triangle's bounding maximum coordinate (max x, max
                                           ///< y, max z)
  PushFeatures push_features{};            ///< Push behavior flags.
  SurfaceFeatures surface_features{};      ///< Surface behavior flags.
};
/**
 * @brief Struct defining a wall edge used in collision detection.
 *
 * This structure seems to be optional and likely serves as an optimization for collision detection with "wall"
 * triangles.
 *
 * @see nnl::collision::CollisionTest
 * @see nnl::collision::Collision::normals
 * @see nnl::collision::Collision::vertices
 */
struct Edge {
  std::array<u16, 2> indices_vertices{0};  ///< Indices of the edge's vertices.
  u16 index_vertex_min = 0;                ///< Index of the edge's bounding minimum coordinate (min x, min
                                           ///< y, min z)
  u16 index_vertex_max = 0;                ///< Index of the edge's bounding maximum coordinate (max x, max
                                           ///< y, max z)
  u16 index_normal = 0;                    ///< Index of the flipped triangle's normal.
  SurfaceFeatures surface_features{};
};
/**
 * @brief Struct representing a set of geometric shapes for collision testing.
 *
 * The CollisionTest struct holds the indices of geometric shapes that are
 * relevant for collision testing at the player's current position.
 *
 * @see nnl::collision::Collision
 */
struct CollisionTest {
  std::vector<u16> triangle_indices;       ///< Holds the indices of triangles
                                           ///< to be tested.
  std::vector<u16> triangle_wall_indices;  ///< Holds the indices of "wall" triangles
                                           ///< to be tested.
  std::vector<u16> edge_wall_indices;      ///< Holds the indices of "wall" edges
                                           ///< to be tested. These seem to be an optional optimization.
};
/**
 * @brief Struct that contains all necessary information for collision
 * detection.
 *
 * The Collision struct contains parameters and data structures necessary for
 * performing collision detection.
 *
 * @note This struct stores only a portion of the data that may be
 * required by a 3D asset to function properly. Its binary representation must
 * be used as part of a larger container that should also include meshes and
 * possibly other data.
 *
 * @see nnl::asset::Asset
 * @see nnl::asset::Asset3D
 * @see nnl::model::Model
 * @see nnl::shadow_collision::Collision
 */
struct Collision {
  u16 shift_value = 5;                         ///< The exponent of 2 that determines how the entire
                                               ///< collision space is divided into smaller parts.
                                               ///< The player's coordinates are shifted by this value
                                               ///< to get a relevant entry in the coordinate_map.
  glm::vec3 origin_point{0.0f};                ///< Represents the origin point in 3D space. All coordinates in
                                               ///< the coordinate_map are assumed to be positive offsets
                                               ///< relative to it.
  std::vector<glm::vec3> vertices;             ///< The vertices that are used to define the geometric
                                               ///< shapes involved in collision detection.
  std::vector<glm::vec3> normals;              ///< Normal vectors associated with the geometric shapes.
  std::vector<Triangle> triangles;             ///< List of triangles.
  std::vector<Edge> edges;                     ///< List of wall edges.
  utl::Array3D<CollisionTest> coordinate_map;  ///< A 3D array that maps coordinates that the player can
                                               ///< end up in to the corresponding collision tests.
                                               ///< ((current_pos - origin_point)>>shift_value)
};
/**
 * @brief Converts collision data from the in-game format to a simplified
 * model representation.
 *
 * @param collision The in-game collision format to be converted.
 * @return The converted model.
 */
SModel Convert(const Collision& collision);
/**
 * @brief Parameters for converting a simplified mesh to the in-game collision
 * format.
 *
 * This struct contains settings that configure collision meshes.
 *
 * @see nnl::SMesh
 */
struct ConvertParam {
  PushFeatures push_features{};
  SurfaceFeatures surface_features{};
};

Collision Convert(SModel&& smodel, const ConvertParam& mesh_params = {}, bool auto_wall = true, u32 shift = 5);
/**
 * @brief Converts a simplified representation of a model to the in-game
 * collision format.
 *
 * @param smodel The source model to be converted.
 * @param mesh_params The parameters that dictate how the conversion should be
 * performed for each mesh.
 * @param auto_wall Automatically set appropriate PushFlags.
 * @param shift The exponent of 2 that determines how the whole collision space
 * is divided. Larger values can reduce the size of data but make collision
 * testing slower.
 * @return The converted collision.
 *
 * @note Use `std::move` when passing an existing object.
 */
Collision Convert(SModel&& smodel, const std::vector<ConvertParam>& mesh_params, bool auto_wall = true, u32 shift = 5);

/**
 * @brief Tests if the provided file is a collision config.
 *
 * This function takes data representing a file and checks
 * whether it corresponds to the in-game collision format.
 *
 * @param buffer The data to be tested.
 * @return Returns true if the file is identified as a collision;
 *
 * @see nnl::collision::Import
 */
bool IsOfType(BufferView buffer);

bool IsOfType(const std::filesystem::path& path);

bool IsOfType(Reader& f);

/**
 * @brief Parses a binary file and converts it to a structured collision.
 *
 * This function takes a binary representation of a collision config, parses its
 * contents, and converts them into a `Collision` struct for easier
 * access and manipulation.
 *
 * @param buffer The binary data to be processed.
 * @return A `Collision` object representing the converted data.
 *
 * @see nnl::collision::IsOfType
 * @see nnl::collision::Export
 */
Collision Import(BufferView buffer);

Collision Import(const std::filesystem::path& path);

Collision Import(Reader& f);

/**
 * @brief Converts a collision config to a binary file representation.
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

/**
 * \defgroup Collision_Auxiliary Auxiliary
 * @ingroup Collision
 * @{
 */
/**
 * @brief Generates conversion parameters for SMesh objects to achieve a similar
 * in-game collision config when converting back to it.

 * @param collision The source in-game collision format from which to generate
 * conversion parameters.
 * @return A vector of ConvertParam objects.
 *
 * @see nnl::collision::Convert
 */
std::vector<ConvertParam> GenerateConvertParam(const Collision& collision);
/** @} */

namespace raw {
/**
 * \defgroup Collision_Raw Raw
 * @ingroup Collision
 * @{
 */

NNL_PACK(struct RHeader {
  Vec4<f32> origin_point{0, 0, 0, 1.0f};  // 0x0
  Vec3<u32> bbox_dimension{0};            // 0x10 width, height, depth shifted by shift_value
  u32 offset_coordinate_map = 0;          // 0x1C
  u32 offset_vertex_table = 0;            // 0x20
  u32 offset_normal_table = 0;            // 0x24
  u32 offset_triangles = 0;               // 0x28
  u32 offset_edges = 0;                   // 0x2C
  u32 offset_collision_tests = 0;         // 0x30
  u32 file_size = 0;                      // 0x34
  u32 unknown = 0;                        // 0x38 always 0; offset to
                                          // unused indices_triangles_edges?
  u16 shift_value = 0;                    // 0x3C
  u16 num_triangles = 0;                  // 0x3E 0 in NUC2
});

static_assert(sizeof(RHeader) == 0x40, "");

NNL_PACK(struct RTriangle {
  u32 calculation = 0;       // 0x0
  u16 index_vertex_0 = 0;    // 0x4
  u16 index_vertex_1 = 0;    // 0x6
  u16 index_vertex_2 = 0;    // 0x8
  u16 index_normal = 0;      // 0xA
  u16 index_vertex_min = 0;  // 0xC
  u16 index_vertex_max = 0;  // 0xE
  u16 push_features = 0;     // 0x10
  u16 surface_features = 0;  // 0x12
});

static_assert(sizeof(RTriangle) == 0x14);  // In NUC2 the size is 0x18(?)

NNL_PACK(struct REdge {
  u32 calculation = 0;       // 0x0
  u16 index_vertex_0 = 0;    // 0x4
  u16 index_vertex_1 = 0;    // 0x6
  u16 index_vertex_min = 0;  // 0x8 //MIN point line
  u16 index_vertex_max = 0;  // 0xA //max point line
  u16 index_normal = 0;      // 0xC
  u16 mask = 0;              // 0xE
});

static_assert(sizeof(REdge) == 0x10);

NNL_PACK(struct RCollisionTest {
  u32 offset_triangle_indices = 0;       // 0x0
  u32 offset_triangle_wall_indices = 0;  // 0x4
  u32 offset_edge_indices = 0;           // 0x8
  u16 num_triangle_indices = 0;          // 0xC
  u16 num_triangle_wall_indices = 0;     // 0xE
  u16 num_edge_indices = 0;              // 0x10
  u16 padding = 0;                       // 0x12
});

static_assert(sizeof(RCollisionTest) == 0x14, "");

struct RCollision {
  RHeader header;
  std::vector<Vec4<f32>> vertices;
  std::vector<Vec4<f32>> normals;
  std::vector<RTriangle> triangles;
  std::vector<REdge> edges;
  std::vector<RCollisionTest> collision_tests;
  std::vector<u16> indices_triangles_edges;  // indices to geometric shapes for collision tests
  std::vector<u16> coordinate_map;           // array[z][y][x]
};

RCollision Parse(Reader& f);

Collision Convert(const RCollision& rcollision);
/** @} */
}  // namespace raw

}  // namespace collision
/** @} */
}  // namespace nnl
