/**
 * @file model.hpp
 * @brief Contains structures and functions for working with in-game
 * 3D models.
 *
 * @see nnl::model::Model
 * @see nnl::model::IsOfType
 * @see nnl::model::Import
 * @see nnl::model::Export
 * @see nnl::model::Convert
 * @see model_common.hpp
 */

#pragma once

#include <array>
#include <cstring>
#include <map>
#include <vector>

#include "NNL/common/enum_flag.hpp"
#include "NNL/common/fixed_type.hpp"
#include "NNL/common/io.hpp"
#include "NNL/game_asset/visual/model_common.hpp"
#include "NNL/simple_asset/smodel.hpp"
#include "NNL/utility/static_set.hpp"

/**
 * \addtogroup Model_Auxiliary
 * @{
 */
#ifdef __DOXYGEN__
/**
 * @def NNL_UVANIM_MAX_DUR
 * @brief The maximum duration of a texture coordinate animation
 *
 * This macro defines the maximum duration of a UV animation when converted
 * from the in-game format to a more generalized format. The default value
 * typically allows for seamless looping of animations within that timeframe.
 * However, it can be changed as needed.
 *
 * @see nnl::model::UVAnimation
 * @see nnl::SUVAnimation
 */
#define NNL_UVANIM_MAX_DUR 1000
#endif
/** @} */

namespace nnl {
/**
 * \addtogroup Model
 * @{
 */

/**
 * @copydoc model.hpp
 *
 */
namespace model {
/**
 * \defgroup Model_Main Main
 * @ingroup Model
 * @{
 */
/**
 * @brief Maximum number of bones that can influence a single
 * primitive and a single vertex.
 *
 * This constant defines the upper limit of bone influences that can be applied
 * to a single vertex and a single primitive. It is set to 8 because the @ref GE
 * can only hold 8 bone matrices in memory at a time when drawing primitives. As
 * the result, a model must be split into submeshes to be drawn.
 *
 * @see nnl::model::SubMesh
 */
constexpr std::size_t kMaxNumBonePerPrim = 8;

/**
 * @brief Specifies the different modes of UV animation for textures.
 *
 * Each mode can be used to create unique visual effects, such as flowing water or moving leaves.
 *
 * @see nnl::model::UVAnimation
 */
enum class UVAnimationMode : u16 {
  kNone = 0,             ///< No shifting occurs, but the animation can
                         ///< still be affected by an action config or the game. In that
                         ///< case, UVAnimation can serve as a placeholder to be targeted.
  kContinuousShift = 1,  ///< In this mode, the u_shift and v_shift values are
                         ///< added to UV coordinates every frame. The following formulas
                         ///< can be used to calculate the absolute u and v translation values:\n
                         ///< u_shift*current_frame\n
                         ///< v_shift*current_frame

  kRangedShift = 2,  ///< In this mode, the UV coordinates are shifted in a
                     ///< circular manner. The u_shift and v_shift define the
                     ///< radius of a shape, and a full shifting cycle takes
                     ///< num_frames (which must be > 0). This can be used to
                     ///< imitate leaf movement.\n
                     ///< u_shift*cos(current_frame*(tau/num_frames))\n
                     ///< v_shift*sin(current_frame*(tau/num_frames))

  kFramedShift = 3  ///< In this mode, the u_shift and v_shift values are added
                    ///< to UV coordinates every nth frame (set with
                    ///< num_frames). This can be used to create short
                    ///< animations with sprite sheets.\n
                    ///< u_shift*int(current_frame/num_frames)\n
                    ///< v_shift*int(current_frame/num_frames)
};

/**
 * @brief Enumeration of supported primitive types.
 *
 * This enum defines various primitive types that can be used in models.
 * Each value in this enumeration maps
 * directly to the corresponding values used by the @ref GE.
 * This library primarily works with triangles, making the
 * kTriangles, kTriangleStrip types particularly
 * relevant. The other types are included for
 * completeness and potential use.
 * However, they are ignored by functions that convert
 * between a simple model representation and the in-game format.
 *
 * @see nnl::model::Primitive
 */
enum class PrimitiveType : u16 {
  kPoints = 0,
  kLines = 1,
  kLineStrip = 2,
  kTriangles = 3,
  kTriangleStrip = 4,
  kTriangleFan = 5,
  kSprites = 6
};
/**
 * @brief Enumeration of material features used in rendering.
 *
 * This enum defines various flags that activate certain features in the
 * rendering system.
 *
 * @note Not all of the flags can be activated at the same time; enabling some of them
 * may cause others to be ignored.
 *
 * @see nnl::model::Material
 */
enum class NNL_FLAG_ENUM MaterialFeatures : u16 {

  kNone = 0,  ///< No material flags are set.

  kVertexColors = 0b000000001,  ///< To use vertex colors with kLit enabled,
                                ///< you should also enable this option.
                                ///< In other cases, vertex colors (if present) will be
                                ///< applied regardless of this flag. Vertex colors replace
                                ///< the ambient component of the material.

  kLit = 0b000000010,                ///< Enables diffuse, specular, and emissive components of
                                     ///< the material. The first two depend on external lights.
                                     ///< The ambient color also becomes dependent on the global
                                     ///< ambient light.
                                     ///< Without this flag, only a texture, vertex colors **or**
                                     ///< the ambient component are used (an unshaded material).
                                     ///< Normals must be present in the vertex data if this flag
                                     ///< is set. @see nnl::lit::Lit
  kProjectionMapping = 0b000000100,  ///< Changes the way a texture is projected onto the mesh.
                                     ///< This mode rotates it towards the camera. Also, it
                                     ///< disables depth writing (not depth testing). The way the
                                     ///< projection looks depends on normals, which must be
                                     ///< present in the vertex data if this flag is set.
  kAdditiveBlending = 0b000001000,   ///< The blend equation is set to add the source
                                     ///< and destination pixel values together
                                     ///< making the output pixels look brighter.

  kSubtractiveBlending = 0b000010000,  ///< The blend equation is set to subtract colors of the
                                       ///< object being rendered _from_ the previously drawn
                                       ///< fragments making them look darker.
  kNoDepthWriteDefer = 0b000100000,    ///< Disables depth writing for this mesh. Note that depth
                                       ///< testing is still performed. It also
                                       ///< causes a mesh to be rendered later (sometimes after
                                       ///< other models). This flag is used for proper alpha blending.
  kAlphaClip = 0b001000000,            ///< Discards rendering of any pixels that are less than 50%
                                       ///< opaque and makes other pixels fully opaque.
  kSkinning = 0b010000000,             ///< @note This flag is included for completeness and must
                                       ///< not be used directly. Instead, set 'use_skinning' in Mesh to
                                       ///< true. @see nnl::model::Mesh
  kEnvironmentMapping = 0b100000000    ///< Changes the way a texture is projected onto the mesh.
                                       ///< Makes it dependent on the cel_shadow_light_direction.
                                       ///< Also, it disables depth
                                       ///< writing. The way the projection looks depends on
                                       ///< normals, which must be present in the vertex data if
                                       ///< this flag is set. @see nnl::lit::Lit
};

NNL_DEFINE_ENUM_OPERATORS(MaterialFeatures)

/**
 * @brief Represents a bone/joint in a skeleton.
 *
 * This struct defines the properties of a bone, as well as its
 * relation to its parent bone in the hierarchy. The transformation properties
 * are relative to the bone's parent. A complete skeleton is represented as an
 * array of such Bone structures (which can be created by performing a preorder
 * traversal of a tree that defines the skeletal hierarchy.)
 *
 * @see nnl::model::Model
 */
struct Bone {
  i16 parent_index = -1;        ///< An index into the array of bones; -1 means a bone has no parent
  glm::vec3 scale{1.0f};        ///< The scale transform.
  glm::vec3 rotation{0.0f};     ///< Pitch, Yaw, Roll. Euler angles in degrees.
                                ///< Each value must be in the range [-180, +180]
  glm::vec3 translation{0.0f};  ///< The translation transform
};
/**
 * @brief Represents a configuration for texture swapping.
 *
 * This struct defines a texture swap configuration that specifies
 * the original texture index and a new texture index that the original texture
 * can be swapped with.
 *
 * A texture swap can be activated by an action configuration or from the
 * game's code. Texture swaps are primarily used to imitate facial animations.
 *
 * @see nnl::model::Model
 * @see nnl::action::ActionConfig
 */
struct TextureSwap {
  i16 original_texture_id = 0;
  i16 new_texture_id = 0;
};
/**
 * @brief Represents a UV animation configuration.
 *
 * This struct defines the properties for UV animation, allowing for
 * various types of texture shifting effects.
 * The behavior of the animation is determined by the
 * specified animation mode, which affects the exact interpretation of
 * num_frames, u_shift, and v_shift.
 *
 * @note At least one instance of this structure must be present in a model
 * (the default value for uv_animation_id is 0). The first instance does not
 * animate anything no matter the settings.
 *
 * @see nnl::model::UVAnimationMode
 * @see nnl::model::Material
 */
struct UVAnimation {
  UVAnimationMode animation_mode = UVAnimationMode::kNone;  ///< Defines how shifting occurs

  u16 num_frames = 0;  ///< The number of frames for the animation.
                       ///< When animation_mode is kRangedShift, this denotes
                       ///< the total duration of a cycle. When animation_mode
                       ///< is kFramedShift, shifting occurs every num_frames.
                       ///< Ignored otherwise.

  f32 u_shift = 0;  ///< When animation_mode is kRangedShift, this denotes
                    ///< the radius of the shape (v_shift should be the same
                    ///< for a circle). In other modes, this value is simply
                    ///< added to texture coordinates as is. This value
                    ///< usually should be in the range (-1.0, 1.0)
  f32 v_shift = 0;  ///< Same as u_shift but for the v coordinate.
};

/**
 * @brief Represents a configuration for attaching an external object to a
 * model.
 *
 * This structure is used to define the transformation properties of a separate
 * 3D asset attached to a bone. The feature is sometimes used in NSLAR
 * cutscenes.
 *
 * @see nnl::model::Model
 */
struct Attachment {
  BoneIndex bone_id = 0;      ///< The index of the bone to which the attachment
                              ///< should be connected.
  u16 attachment_id = 0;      ///< A number that serves as the identifier for this
                              ///< struct. The game looks for this id.
  glm::mat4 transform{1.0f};  ///< A transformation matrix for the
                              ///< attachment (relative to the parent bone)
};

/**
 * @brief Represents a bounding box for objects.
 *
 * This structure defines a bounding box that is used for frustum
 * culling of map objects (it has no effect on characters)
 *
 * The bounding box is defined by two vertices, which represent the corners of
 * the box in 3D space.

 * @note The vertices must be pre-transformed if necessary. The BBox
 * inherits the transform of its parent bone; however, an inverse matrix is not applied
 * to it automatically.
 *
 * @see nnl::model::Mesh
 * @see nnl::model::GenerateBBox
 */
struct BBox {
  BoneIndex bone_index = 0;  ///< Index of the bone the BBox is attached to
                             ///< (usually the root bone).
  glm::vec3 min{0.0f};       ///< The min coordinate
  glm::vec3 max{0.0f};       ///< The max coordinate
};

/**
 * @brief Represents the smallest unit of geometry in the hierarchy.
 *
 * The Primitive struct encapsulates details about the primitive type used, the
 * number of elements contained, and the associated data buffer. For instance,
 * when using PrimitiveType::kTriangles, the buffer may contain 3 or **more** vertices to
 * represent one or **multiple** triangles.
 *
 * @see nnl::model::SubMesh
 * @see nnl::vertexde::Decode
 */
struct Primitive {
  PrimitiveType primitive_type = PrimitiveType::kTriangles;  ///< Type of the primitive.
  u16 num_elements = 0;                                      ///< Represents the number of vertices **or** indices
                                                             ///< stored in the vertex_index_buffer.
  Buffer vertex_index_buffer;                                ///< Stores either vertices or indices
                                                             ///< (determined by vertex_format).
                                                             ///< If indexed drawing is used, the
                                                             ///< indices refer to the `indexed_vertex_buffer` from
                                                             ///< the parent SubMesh.
};
/**
 * @brief Represents a smaller part of a mesh.
 *
 * This struct represents a smaller part of a mesh. It exists mainly due to the
 * fact that the PSP's Graphics Engine can hold only 8 skin matrices at a time
 * when rendering a Primitive. Therefore, primitives that share bones need to be
 * grouped into these structs to be drawn. There is usually only 1 such struct if a mesh
 * does not use skinning. If a mesh is too big to fit into 1 submesh, it can
 * also be sliced into multiple parts.
 *
 * @note A submesh is always influenced by at least 1 bone. Therefore, the skeleton must
 * contain at least 1 entry, which is essential for positioning the object in the world.
 * It is **not** necessary to include weights into the vertex data when a submesh
 * is influenced by only 1 bone. \n\n
 *
 * @note In **NSUNI**, it is possible to set num_bones to 0
 * in a specific edge case. If a mesh is influenced by multiple bones and
 * uses multiple weights, the flag use_skinning must be set to true.
 * However, when slicing it into submeshes, some of them may end up
 * using only 1 bone. In this situation, to **omit** the inclusion of 1
 * weight into the vertex data, num_bones may be set to 0. However, the first
 * bone index will be used anyway. This approach is **not** supported by NSLAR and
 * will lead to partial rendering of the model.
 *
 * @see nnl::model::Mesh
 */
struct SubMesh {
  u16 num_bones = 1;  ///< The number of bones this submesh refers to.
                      ///< Typically, this value must be within the range of 1
                      ///< to 8 (0 is allowed in a specific edge case)

  u32 vertex_format = 0;              ///< The format of the vertex data. It matches what
                                      ///< the @ref GE expects @see nnl::vertexde::fmt
  Buffer indexed_vertex_buffer;       ///< If vertex_format is set to use indices, this
                                      ///< buffer must store unique vertices. In that
                                      ///< case, buffers in primitives store
                                      ///< indices that refer to this buffer. It's empty otherwise.
  std::vector<Primitive> primitives;  ///< List of primitives that make up this submesh.

  std::array<BoneIndex, kMaxNumBonePerPrim> bone_indices{0};  ///< Indices of the bones influencing this submesh.
                                                              ///< Every bone used by submeshes **must**
                                                              ///< have a corresponding entry in the
                                                              ///< inverse_matrix_bone_table
                                                              ///< @see nnl::model::Model::inverse_matrix_bone_table
  std::vector<u32> display_list;                              ///< A list of rendering commands for drawing
                                                              ///< the primitives in this submesh. It must
                                                              ///< contain at least 4 basic commands per
                                                              ///< each primitive. Additional commands may
  ///< also be inserted, in which case they must be located
  ///< right before the draw command or after it (otherwise,
  ///< the games may alter the list). @note These display
  ///< commands are only effective when indexed drawing is
  ///< **not** used. If indexed drawing is enabled, this list
  ///< is ignored by the games. @see nnl::model::GeCmd
};
/**
 * @brief This struct defines various properties of a mesh.
 *
 *
 * @see nnl::model::MeshGroup
 */
struct Mesh {
  std::vector<SubMesh> submeshes;  ///< A collection of submeshes that make up
                                   ///< the mesh (1 or more)
  bool use_skinning = false;       ///< Flag indicating whether skinning is applied
                                   ///< to the mesh. It should be active for meshes
                                   ///< that **include** weights into their vertex data
                                   ///< and are influenced by **multiple** bones.
                                   ///< In other cases, it should be set to false.
  bool use_bbox = false;           ///< Flag indicating whether to use the bounding box.

  BBox bbox;            ///< The bounding box of the mesh
  u16 material_id = 0;  ///< The index of the material associated with this mesh.
};
/**
 * @brief Represents a group of meshes.
 *
 * This struct groups multiple meshes together. The visibility
 * of these groups may be animated. This struct also enables texture coordinate
 * adjustments, which are used to restore the origianl UV coordinates when they
 * are stored as normalized integers.
 *
 * @note The visibility of a group can be influenced by multiple external
 * sources at once: an action config, the game's code, a dedicated visibility
 * animation file.
 *
 * @see nnl::model::Mesh
 * @see nnl::visanimation::AnimationContainer
 * @see nnl::action::ActionConfig
 */
struct MeshGroup {
  std::vector<Mesh> meshes;  ///< Collection of meshes that belong to this group.
  f32 u_scale = 1.0f;        ///< Scale factor for the U texture coordinate.
  f32 v_scale = 1.0f;        ///< Scale factor for the V texture coordinate.
  f32 u_offset = 0.0f;       ///< Offset for the U texture coordinate.
  f32 v_offset = 0.0f;       ///< Offset for the V texture coordinate.
};
/**
 * @brief Represents the properties of a material.
 *
 * This struct defines various attributes of a material that determine the look of
 * the mesh that uses it.
 *
 * By default, the material does not interact with lights and only the ambient
 * component, vertex colors, and a texture are used. To change that the
 * kLit flag may be activated, which enables the use of the Phong
 * reflection model.\n
 * The alpha component of the ambient color can be utilized
 * for setting transparency. The specular component is often unused and
 * invisible unless the light source configuration is changed.
 *
 * @see nnl::model::MaterialFeatures
 * @see nnl::lit::Lit
 */
struct Material {
  u32 specular = 0xFF000000;   ///< Specular color of the material in the ABGR
                               ///< format (alpha is ignored).
  u32 diffuse = 0xFFFFFFFF;    ///< Diffuse color of the material in the ABGR
                               ///< format (alpha is ignored).
  u32 ambient = 0xFF808080;    ///< Ambient/constant color of the material in the
                               ///< ABGR format (alpha is **used**).
  u32 emissive = 0xFF000000;   ///< Emissive color of the material in the ABGR
                               ///< format (alpha is ignored).
  f32 specular_power = 50.0f;  ///< Power of the specular highlight.
  i16 texture_id = -1;         ///< Index of the texture; -1 indicates no texture.
  u8 uv_animation_id = 0;      ///< Index of UV animation; 0 indicates no animation.
                               ///< @see nnl::model::UVAnimation
  u8 vfx_group_id = 0;         ///< Identifier for the material's visual effects group.
                        ///< Various effects can be applied to meshes of that group by the game. For instance, in NSUNI,
                        ///< in character assets, a value of 1 is used to make "outline" meshes black and
                        ///< "silhouette" meshes (when a character runs) semi-transparent.
  MaterialFeatures features{};  ///< Render settings for the material
};

/**
 * @brief Represents a 3D model with various components.
 *
 * @note This struct usually stores only a portion of the data required by a
 * 3D asset to function properly. Its binary representation must be used as part
 * of a larger container that may also include textures, animations and other
 * data.
 *
 * @see nnl::asset::Asset
 * @see nnl::asset::Asset3D
 * @see nnl::texture::TextureContainer
 * @see nnl::animation::AnimationContainer
 * @see nnl::action::ActionConfig
 * @see nnl::colbox::ColBoxConfig
 * @see nnl::visanimation::AnimationContainer
 * @see nnl::collision::Collision
 * @see nnl::shadow_collision::Collision
 */
struct Model {
  std::vector<Bone> skeleton;  ///< Array of bones that define the skeleton. It must contain at least one bone.
                               ///< This array may be derived from a preorder traversal of a skeletal tree hierarchy.

  bool move_with_root = false;  ///< Indicates whether the model should be repositioned in the
                                ///< world when the root bone is moved by an animation.

  std::map<BoneIndex, glm::mat4> inverse_matrix_bone_table;  ///< A table that maps bone indices to their inverse bind
                                                             ///< matrices. An entry is required **only
                                                             ///< if** a mesh is attached to that bone. The matrix may
                                                             ///< be a simple inverse or combined with scaling and other
                                                             ///< transformations - this is often the case when
                                                             ///< positions are stored as normalized integers.

  std::vector<std::map<BoneTarget, BoneIndex>> bone_target_tables;  ///< An optional collection of bone target tables.
                                                                    ///< Each table maps generic numeric IDs to concrete
                                                                    ///< bone indices, enabling the game to target
                                                                    ///< specific bones. For example, this allows the
                                                                    ///< game to identify which bone corresponds to the
                                                                    ///< right hand for attaching effects. There are at
                                                                    ///< most 2 such tables present in game assets. 1
                                                                    ///< table is usually present in character assets. 2
                                                                    ///< tables are sometimes used in effect assets.
                                                                    ///< @see nnl::model::BoneTarget
                                                                    ///< @see nnl::model::HumanoidRigTarget

  std::vector<TextureSwap> texture_swaps;  ///< A collection of texture swaps
                                           ///< that may be used in animations.

  std::vector<Attachment> attachments;  ///< A collection of attachment configs for
                                        ///< external objects.

  std::vector<UVAnimation> uv_animations;  ///< A vector of UV animations for the model. This vector
                                           ///< must contain at least one default entry.

  std::vector<MeshGroup> mesh_groups;  ///< A collection of mesh groups with meshes that make up the
                                       ///< model. Typically, there is at least one mesh group.

  std::vector<Material> materials;  ///< A collection of materials used by the model. This vector
                                    ///< must contain at least one material.
};

/**
 * @brief Enum representing compression levels for mesh conversion.
 *
 * This enum defines the levels of compression applied to vertex attributes when
 * converting from a simplified mesh to the in-game Mesh format. The selected
 * compression level affects the size and quality of the resulting mesh.
 *
 * @see nnl::model::ConvertParam
 */
enum class CompLvl {
  kNone = 0,    ///< No compression applied. Uses float types and RGBA8888 format.
  kMedium = 1,  ///< Uses shorts and bytes for attributes, RGBA4444 for color.
  kMax = 2      ///< Uses bytes for maximum compression, RGBA4444 for color.
                ///< Suitable only for basic meshes.
};

/**
 * @brief Parameters for converting a simplified mesh to the in-game format.
 *
 * @see nnl::model::Convert
 * @see nnl::SMesh
 */
struct ConvertParam {
  CompLvl compress_lvl = CompLvl::kMedium;  ///< The level of compression to apply to vertex
                                            ///< data.

  u32 force_vertex_format = 0;    ///< If not 0, this value forces the specified vertex format.
                                  ///< Otherwise, the format is determined by compress_lvl
                                  ///< and the attributes used by the source mesh.
  bool indexed = true;            ///< Indicates whether the converted mesh should use
                                  ///< indexed rendering.
  bool use_bbox = false;          ///< Specifies if a BBox should be generated and used
                                  ///< for the mesh.
  bool optimize_weights = false;  ///< Only for NSUNI! Excludes weights from vertex
                                  ///< data in a few cases @see nnl::model::SubMesh
  bool use_strips = true;         ///< Specifies whether to use triangles or triangle strips
  bool stitch_strips = true;      ///< Indicates whether to stitch triangle strips
                                  ///< using 0 area triangles.
  bool join_submeshes = true;     ///< Specifies whether to reduce the number of SubMeshes by joining
                                  ///< some of them. This may reduce the number of draw calls.
};

/**
 * @brief Converts a simplified model representation to the in-game format.
 *
 * This function transforms an `SModel` into a `Model`.
 *
 * @param smodel The simplified model representation to be converted.
 * @param mesh_params Parameters for each individual mesh conversion.
 * @param move_with_root A boolean flag indicating whether the model should
 *                       move with the root bone.
 * @return A `Model` representing the converted in-game format.
 *
 * @note Use `std::move` when passing an existing object.
 */
Model Convert(SModel&& smodel, const std::vector<ConvertParam>& mesh_params, bool move_with_root = false);
/**
 * @brief Converts a simplified model representation to the in-game format.
 *
 * This function transforms an `SModel` into a `Model`.
 *
 * @param smodel The simplified model representation to be converted.
 * @param mesh_params Parameters for mesh conversion.
 * @param move_with_root A boolean flag indicating whether the model should
 *                       move with the root bone.
 * @return A `Model` representing the converted in-game format.
 *
 * @note Use `std::move` when passing an existing object.
 */
Model Convert(SModel&& smodel, const ConvertParam& mesh_params = {}, bool move_with_root = false);
/**
 * @brief Converts a game model to a simplified model representation.
 *
 * This function transforms a `Model` into an `SModel`,
 * which is a simplified representation suitable for processing or for exporting
 * into other common formats such as FBX or glTF
 *
 * @param model The game model to be converted.
 * @return An `SModel`.
 */
SModel Convert(const Model& model);

/**
 * @brief Tests if the provided file is a model.
 *
 * This function takes data representing a file and checks
 * whether it corresponds to the in-game model format.
 *
 * @param buffer The file buffer to be tested.
 * @return Returns true if the file is identified as a model;
 *
 * @see nnl::model::Import
 */
bool IsOfType(BufferView buffer);

bool IsOfType(const std::filesystem::path& path);

bool IsOfType(Reader& f);

/**
 * @brief Parses a binary file and converts it to a structured model.
 *
 * This function takes a binary representation of a model, parses its
 * contents, and converts them into a `Model` struct for easier
 * access and manipulation.
 *
 * @param buffer The file buffer to be processed.
 * @return A `Model` object representing the converted data.
 *
 * @see nnl::model::IsOfType
 * @see nnl::model::Export
 */
Model Import(BufferView buffer);

Model Import(const std::filesystem::path& path);

Model Import(Reader& f);
/**
 * @brief Converts a model to its binary file representation.
 *
 * This function takes a `Model` object and converts it into the binary format.
 *
 * @param model The `Model` object to be converted.
 * @return A buffer containing the binary representation of the model.
 */
[[nodiscard]] Buffer Export(const Model& model);

void Export(const Model& model, const std::filesystem::path& path);

void Export(const Model& model, Writer& f);
/** @} */  // Main

/**
 * \defgroup Model_Auxiliary Auxiliary
 * @ingroup Model
 * @{
 */

/**
 * @brief Enumeration of _essential_ rendering commands for the library.
 *
 * This enum defines a **few** rendering commands used by the Graphics Engine (@ref GE).
 *
 * Each command is represented by a 32-bit value, where the first byte
 * indicates the command, and the remaining bytes store its arguments.
 */
enum GeCmd : u32 {
  kReturn = 0x0B'00'00'00,          ///< Marks the end of the display list.
  kSetVertexType = 0x12'00'00'00,   ///< Sets the vertex format. Indicates the rendering modes, the attributes
                                    ///< used, their formats @see nnl::vertexde::fmt
  kSetBaseAddress = 0x10'00'00'00,  ///< Sets the base address;
                                    ///< The command must be present in a display list.
                                    ///< Arguments are filled dynamically by the game engine.
  kSetVramAddress = 0x01'00'00'00,  ///< Sets the address for vertex data; The command must be
                                    ///< present in a display list. Arguments are filled
                                    ///< dynamically by the game engine.
  kDrawPrimitives = 0x04'00'00'00,  ///< Command to draw primitives based on the current
                                    ///< vertex data. Expects a primitve type (8 bits) and a
                                    ///< number of elements as its arguments (last 16 bits).
                                    ///< Number of elements is a number of vertices or
                                    ///< indices. @see nnl::model::PrimitiveType
};

/**
 * @brief Enum representing parts of a generic humanoid rig.
 *
 * This enum defines _generic_ identifiers for different parts of a humanoid
 * rig. These identifiers can be used by the games to access specific bones in a skeleton
 * for procedural animation and effect attachment.
 *
 * @see nnl::model::BoneTarget
 * @see nnl::model::Model::bone_target_tables
 */
enum HumanoidRigTarget : BoneTarget {
  kRoot = 0,
  kHips = 1,
  kSpine = 2,
  kSpine1 = 3,
  kNeck = 4,
  kHead = 5,
  kLeftShoulder = 6,
  kLeftArm = 7,
  kLeftForeArm = 8,
  kLeftHand = 9,
  kRightShoulder = 0xA,
  kRightArm = 0xB,
  kRightForeArm = 0xC,
  kRightHand = 0xD,
  kLeftUpLeg = 0xE,
  kLeftLeg = 0xF,
  kLeftFoot = 0x10,
  kRightUpLeg = 0x11,
  kRightLeg = 0x12,
  kRightFoot = 0x13,
  // the rest might vary depending on the asset
  kBodyPart20 = 0x14,
  kBodyPart21 = 0x15,
  kBodyPart22 = 0x16,
  kBodyPart23 = 0x17,
  kBodyPart24 = 0x18,
  kBodyPart25 = 0x19,
  kBodyPart26 = 0x1A,
  kBodyPart27 = 0x1B,
  kBodyPart28 = 0x1C,
  kBodyPart29 = 0x1D,
};

/**
 * @brief An array that associates a generic bone id with a name.
 *
 * @see nnl::model::BoneTarget
 * @see nnl::model::GenerateBoneTargetTables
 * @see nnl::model::SetBoneNames
 * @see nnl::model::HumanoidRigTarget
 */
constexpr std::array<std::string_view, 30> bone_target_names{
    "Root",         "Hips",       "Spine",       "Spine1",     "Neck",          "Head",
    "LeftShoulder", "LeftArm",    "LeftForeArm", "LeftHand",   "RightShoulder", "RightArm",
    "RightForeArm", "RightHand",  "LeftUpLeg",   "LeftLeg",    "LeftFoot",      "RightUpLeg",
    "RightLeg",     "RightFoot",  "BodyPart20",  "BodyPart21", "BodyPart22",    "BodyPart23",
    "BodyPart24",   "BodyPart25", "BodyPart26",  "BodyPart27", "BodyPart28",    "BodyPart29"};

/**
 * @brief Extracts triangles from a given submesh.
 *
 * This function processes the given submesh, which can be in multiple
 * formats, to extract its triangles into a simpler representation.
 *
 * @param submesh The submesh from which to extract triangles.
 * @param skip_degenerate A boolean flag indicating whether to skip 0 area
 * triangles.
 * @return A list of triangles
 */
std::vector<STriangle> ExtractTriangles(const SubMesh& submesh, bool skip_degenerate = true);
/**
 * @brief Represents a group of triangles associated with specific bones.
 *
 * This type consists of unique bone IDs and a vector of triangle
 * indices.
 *
 * @see nnl::model::GroupTrianglesByBones
 */
struct TriangleGroup {
  utl::StaticSet<BoneIndex, kMaxNumBonePerPrim> bones;  ///< Unique bones
  std::vector<u32> indices;                             ///< Triangles associated with the bones
};
/**
 * @brief Groups triangles by their associated bones.
 *
 * This function slices triangles into groups based on the bones that influence
 * them. These groups are used to create SubMeshes.
 *
 * @param indices A vector of triangle indices to be grouped.
 * @param vertices A vector of vertices associated with the triangles.
 * @param join_bone_sets Whether to join different bone sets
 * into larger ones. This may reduce the number of draw calls but increase the
 * size of an asset.
 * @param max_num_triangles The maximum number of triangles allowed in a group.
 * The choice of this number depends on the index type and the primitive type
 * a SubMesh uses.
 * @return A vector of TriangleGroup objects.
 */
std::vector<TriangleGroup> GroupTrianglesByBones(const std::vector<u32>& indices, const std::vector<SVertex>& vertices,
                                                 u32 max_num_triangles = std::numeric_limits<u16>::max() / 3,
                                                 bool join_bone_sets = true);
/**
 * @brief Generates an axis-aligned bounding box (AABB) for the given mesh.
 *
 * This function computes the bounding box,
 * and applies the provided transformation matrix to it. The resulting
 * bounding box is represented as a `BBox` object.
 *
 * @param mesh The mesh for which to generate the bounding box.
 * @param transform A transformation matrix to be applied to the bbox.
 * @return The generated BBox object.
 */
BBox GenerateBBox(const SMesh& mesh, glm::mat4 transform = glm::mat4(1.0f));

/**
 * @brief Converts a simplified representation of a mesh to the in-game format.
 *
 * @param smesh The source mesh to be converted.
 * @param param The parameters that dictate how the conversion should be
 * performed.
 * @return The converted mesh.
 *
 * @note Use `std::move` when passing an existing object.
 */
Mesh Convert(SMesh&& smesh, const ConvertParam& param = {});
/**
 * @brief Converts a mesh from the in-game format to a simplified
 * representation.
 *
 * @param mesh The mesh to be converted
 * @return The converted mesh.
 */
SMesh Convert(const Mesh& mesh);
/**
 * @brief Generates a conversion parameter for SMesh to get a similar
 * in-game Mesh when converting back to it.
 *
 *
 * @param mesh The source mesh in the in-game format from which to generate
 * conversion parameters.
 * @return A ConvertParam object.
 */
ConvertParam GenerateConvertParam(const Mesh& mesh);
/**
 * @brief Generates a vector of conversion parameters for the entire model
 * to achieve a similar in-game representation.
 *
 * This function attempts to create a collection of `ConvertParam` objects
 * that would result in a similar in-game model when converting from a
 * source model (`SModel`) back to the in-game format (`Model`).
 *
 * @param model The source model from which to generate conversion parameters.
 * @return A vector of ConvertParam objects
 */
std::vector<ConvertParam> GenerateConvertParam(const Model& model);
/**
 * @brief Converts a skeleton represented as a tree into a vector of bones.
 *
 * This function takes an `SSkeleton` and converts it
 * into a vector of Bone structures.
 *
 * @param skeleton The input skeleton.
 * @return A vector of Bone structures representing the converted skeleton.
 */
std::vector<Bone> Convert(const SSkeleton& skeleton);

/**
 * @brief Assigns meaningful names to bones in a skeleton.
 *
 * This function assigns more meaningful names to the bones in the provided
 * SSkeleton based on the specified `bone_target_tables`.
 *
 * The new names can also be used to recreate the source `bone_target_tables`.
 *
 * @note A single bone may be associated with multiple BoneTarget's in which case
 * an underscore (_) is used to separate them. Those targets may also belong to different tables
 * in which case a vertical bar (|) is used to split them.
 *
 * @param skeleton An `SSkeleton` whose bones will be
 *                 renamed.
 * @param bone_target_tables Tables that associate bone targets
 * with concrete bone indices. This information is used to determine the
 * appropriate names for each bone.
 * @param is_character A flag indicating whether to use a humanoid bone naming
 *                  convention. If false, bone target names will consist of the
 * prefix VFX followed by a target id.
 *
 * @see nnl::model::BoneTarget
 * @see nnl::model::GenerateBoneTargetTables
 * @see nnl::model::HumanoidRigTarget
 * @see nnl::model::bone_target_names
 */
void SetBoneNames(SSkeleton& skeleton, const std::vector<std::map<BoneTarget, BoneIndex>>& bone_target_tables,
                  bool is_character = true);
/**
 * @brief Generates bone target tables from bone names of the skeleton.
 *
 * This function creates a vector of tables that associate bone targets with
 * concrete bone indices based on the bone names of the provided
 * `SSkeleton`.
 *
 * @param skeleton The input `SSkeleton` from which the bone target
 *                 tables will be generated.
 * @return A vector of tables linking bone targets to bone indices.
 *
 * @see nnl::model::BoneTarget
 * @see nnl::model::SetBoneNames
 */
std::vector<std::map<BoneTarget, BoneIndex>> GenerateBoneTargetTables(const SSkeleton& skeleton);
/**
 * @brief Converts a vector of bones into a simple tree-like skeleton
 * representation.
 *
 * This function transforms a vector of `Bone` structures into an
 * `SSkeleton` object, organizing the bones into a hierarchical structure.
 *
 * @param bones A vector of `Bone` structures to be converted into a skeleton.
 * @return An `SSkeleton` that represents the hierarchical skeleton
 * structure.
 */
SSkeleton Convert(const std::vector<Bone>& bones);

/**
 * @brief Generates a map of bone indices and inverse matrices.
 *
 * This function creates a map of bone indices and their corresponding inverse
 * matrices. Note that an entry in this table can be omitted if there is no
 * meshes attached to the corresponding bone.
 *
 * @param smodel The input `SModel` containing meshes and a skeleton.
 * @return A map where each entry consists of a bone index and its corresponding
 *         inverse matrix.
 *
 * @see nnl::model::Model::inverse_matrix_bone_table
 */
std::map<BoneIndex, glm::mat4> GenerateInverseMatrixBoneTable(const SModel& smodel);

/**
 * @brief Converts a simple UV animation _channel_ to an in-game UVAnimation config.
 *
 *
 * @param suv_anim_channel
 * @return UVAnimaion
 *
 * @note SUVChannel represents a generic texture coordinate animation.
 * Only a few types of animation can be detected and converted into a UVAnimation config.
 */
UVAnimation Convert(const SUVChannel& suv_anim_channel);

/**
 * @brief Converts an in-game UVAnimation config to a simple UV animation _channel_.
 *
 * @param uv_anim
 * @return SUVChannel
 */
SUVChannel Convert(const UVAnimation& uv_anim);

/**
 * @brief Converts SMaterial to Material.
 *
 * @param smat
 * @return Material
 */
Material Convert(const SMaterial& smat);

/**
 * @brief Converts Material to SMaterial.
 *
 * @param mat
 * @return Material
 */
SMaterial Convert(const Material& mat);

/** @} */  // Auxiliary

namespace raw {

/**
 * \defgroup Model_Raw Raw
 * @ingroup Model
 *
 * The `raw` namespace contains structures that define
 * the unprocessed binary representation of model data and functions to work
 * with it. This includes low-level data structures that are used to serialize
 * and deserialize information directly from binary files.
 *
 * The intricate representations in the raw namespace are simplified in the
 * main `model` namespace to provide a more user-friendly interface.
 * @{
 */

/**
 * @brief Magic bytes used to identify the model format.
 *
 * This constant represents a unique identifier that model files start with.
 */
constexpr u32 kMagicBytes = 0x16'81'00'01;

NNL_PACK(struct RHeader {
  u32 magic_bytes = kMagicBytes;
  u32 offset_bones = 0;                      // 0x4;
  u32 offset_inverse_matrices = 0;           // 0x8
  u32 offset_inverse_matrix_bone_table = 0;  // 0xC
  u32 offset_mesh_groups = 0;                // 0x10
  u16 num_bones = 0;                         // 0x14
  u16 num_mesh_groups = 0;                   // 0x16
  u16 num_inverse_matrices = 0;              // 0x18
  u16 num_vfx_groups = 0;                    // 0x1A
  u32 header_size = sizeof(RHeader);         // 0x1C probably...
  u16 move_with_root = 0;                    // 0x20
  u16 num_bone_target_tables = 0;            // 0x22; 2 is max
  u32 offset_bone_target_tables = 0;         // 0x24
  u16 num_attachment_matrices = 0;           // 0x28;
  u16 num_uv_animations = 0;                 // 0x2A
  u32 offset_attachment_matrices = 0;        // 0x2C
  u32 offset_uv_animations = 0;              // 0x30

  u16 num_textures = 0;          // 0x34 total number of used textures
  u16 num_texture_swaps = 0;     // 0x36
  u32 offset_texture_swaps = 0;  // 0x38
  u32 address = 0;               // 0x3C memory location, filled dynamically
});

static_assert(sizeof(RHeader) == 0x40);

NNL_PACK(struct RBone {
  Vec3<f32> scale{0.0f};
  i16 parent_index = 0;
  Vec3<i16> rotation{0};  // pitch; yaw; roll; as normalized ints
  Vec3<f32> translation{0.0f};
});

static_assert(sizeof(RBone) == 0x20);

NNL_PACK(struct RBBox {
  u16 bone_index = 0;
  u8 padding[0xE] = {0};
  Vec4<f32> vertices[8] = {0.0f};
  // 0 minX, minY, minZ
  // 1 maxX, minY, minZ
  // 2 minX, maxY, minZ
  // 3 maxX, maxY, minZ
  // 4 minX, minY, maxZ
  // 5 maxX, minY, maxZ
  // 6 minX, maxY, maxZ
  // 7 maxX, maxY, maxZ
});

static_assert(sizeof(RBBox) == 0x90);

NNL_PACK(struct RAttachment {
  u16 bone_id = 0;
  u16 attachment_id = 0;
});

static_assert(sizeof(RAttachment) == 0x4);

NNL_PACK(struct RUVAnimation {
  u16 animation_mode = 0;
  u16 num_frames = 0;
  f32 u_shift = 0;
  f32 v_shift = 0;
  u32 unknown = 0;  // reserved (?)
});

static_assert(sizeof(RUVAnimation) == 0x10);

NNL_PACK(struct RTextureSwap {
  u32 id = 0;  // not used
  i16 original_texture_id = 0;
  i16 new_texture_id = 0;
});

static_assert(sizeof(RTextureSwap) == 0x8);

NNL_PACK(struct RBoneTargetHeader {
  u32 offset = 0;
  u32 num_entries = 0;
});

static_assert(sizeof(RBoneTargetHeader) == 0x8);

NNL_PACK(struct RBoneTargetEntry {
  u16 bone_index = 0;
  u16 role_id = 0;
});

static_assert(sizeof(RBoneTargetEntry) == 0x4);

struct RBoneTargetTables {
  std::vector<RBoneTargetHeader> header;
  std::vector<std::vector<RBoneTargetEntry>> entries;
};

NNL_PACK(struct RMeshGroup {
  u32 offset_meshes = 0;  // 0x0
  u16 num_meshes = 0;     // 0x4
  u16 padding = 0;        // 0x6
  f32 u_scale = 0.0f;     // 0x8
  f32 v_scale = 0.0f;     // 0xC
  f32 u_offset = 0.0f;    // 0x10
  f32 v_offset = 0.0f;    // 0x14
});

static_assert(sizeof(RMeshGroup) == 0x18);

NNL_PACK(struct RMesh {
  u32 offset_submeshes = 0;   // 0x0
  u16 num_submeshes = 0;      // 0x24
  u16 material_features = 0;  // 0x6
  u32 specular = 0;           // 0x8
  u32 diffuse = 0;            // 0xC
  u32 ambient = 0;            // 0x10
  u32 emissive = 0;           // 0x14
  f32 specular_power = 0.0f;  // 0x18
  i16 texture_id = 0;         // 0x1C
  u16 use_bbox = false;       // 0x1E
  u32 offset_bbox = 0;        // 0x20 if not used, set to the first offset to submeshes
  u16 material_id = 0;        // 0x24 note: there is no separate array of
                              // materials in the binary format. This number is used to
                              // determine if the next mesh uses the same material or not
  u8 vfx_group_id = 0;        // 0x26
  u8 uv_animation_id = 0;     // 0x27
});

static_assert(sizeof(RMesh) == 0x28);

NNL_PACK(struct RSubMesh {
  u16 num_bones = 0;       // 0x0
  u16 num_primitives = 0;  // 0x2

  u32 vertex_format = 0;                               // 0x4
  u32 offset_indexed_vertex_buffer = 0;                // 0x8
  u32 offset_primitives = 0;                           // 0xC
  u16 inv_mat_bone_indices[kMaxNumBonePerPrim] = {0};  // 0x10 note: entries in the "inverse matrix to bone" table!
  u32 offset_display_list = 0;                         // 0x20
});

static_assert(sizeof(RSubMesh) == 0x24);

NNL_PACK(struct RPrimitive {
  u16 primitive_type = 0;
  u16 num_elements = 0;
  u32 offset_buffer = 0;           // offset to vertex buffer or to index buffer
  u32 buffer_size = 0;             // aligned to 0x40
  u32 ptr_vram_vertex_buffer = 0;  // set by the game
});

static_assert(sizeof(RPrimitive) == 0x10);

struct RModel {
  RHeader header;

  std::vector<RBone> bones;
  // padding to 0x10
  std::vector<Mat4<f32>> inverse_matrices;
  std::vector<RTextureSwap> texture_swaps;
  std::vector<u16> inverse_matrix_bone_table;
  // padding to 0x4
  RBoneTargetTables bone_target_tables;
  // padding to 0x10
  std::vector<Mat4<f32>> attachment_matrices;
  std::vector<RAttachment> attachments;
  // padding to 0x4
  std::vector<RUVAnimation> uv_animations;
  // padding to 0x10
  std::vector<RMeshGroup> mesh_groups;
  // Maps of offsets (used in the structures above) to their data:
  std::map<u32, std::vector<RMesh>> meshes;
  std::map<u32, RBBox> bboxes;
  std::map<u32, std::vector<RSubMesh>> submeshes;
  std::map<u32, std::vector<RPrimitive>> primitives;
  std::map<u32, std::vector<u32>> display_lists;
  std::map<u32, Buffer> vertex_buffers;          // stores vertex buffers or
                                                 // index buffers
  std::map<u32, Buffer> indexed_vertex_buffers;  // stores vertex buffers
                                                 // when indexed drawing is used
};

std::vector<Bone> Convert(const std::vector<raw::RBone>& rbones);

Model Convert(RModel&& rmodel);

RModel Parse(Reader& f);

std::vector<raw::RBone> Convert(const std::vector<Bone>& bones);

/** @} */  // Raw
}  // namespace raw

}  // namespace model
/** @} */  // Model
}  // namespace nnl
