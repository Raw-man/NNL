/**
 * @file smodel.hpp
 * @brief Provides data structures and functions for managing essential components of a 3D
 * model.
 *
 * This file defines structures for the essential components of 3D
 * models. These structures facilitate the conversion of 3D model data between
 * various common exchange formats (such as FBX and glTF) and in-game formats.
 *
 * @see nnl::SModel
 * @see nnl::model::Model
 */
#pragma once

#include <functional>
#include <glm/glm.hpp>
#include <string>
#include <vector>

#include "NNL/common/enum_flag.hpp"
#include "NNL/common/fixed_type.hpp"
#include "NNL/simple_asset/sanimation.hpp"
#include "NNL/simple_asset/svalue.hpp"

namespace nnl {

/**
 * \defgroup SModel Simple Model
 * @ingroup Simple3DAsset
 * @copydoc smodel.hpp
 * @{
 */

constexpr std::size_t kMaxNumVertWeight = 8;
/**
 * @brief Represents a simple vertex with a fixed set of attributes.
 *
 * This structure encapsulates the essential attributes of a vertex. It is used
 * for exporting and importing vertex data to and from common exchange formats.
 * It can also be utilized for converting vertex data to vertex buffers
 * supported by the @ref GE.
 *
 * @note A right-handed coordinate system is used where the X-axis points to the
 * right, the Y-axis points up, and the Z-axis points out of the screen.
 *
 * @see nnl::vertexde::Encode
 * @see nnl::vertexde::Decode
 * @see nnl::SMesh
 * @see nnl::SModel
 * @see nnl::model::Export
 * @see nnl::model::Import
 */
struct SVertex {
  glm::vec3 position{0.0f};  ///< The position of the vertex in 3D space.

  glm::vec3 normal{0.0f, 1.0f, 0.0f};  ///< A vertex normal.

  glm::vec2 uv{0.0f};  ///< Texture coordinates.

  glm::vec4 color{1.0f, 1.0f, 1.0f, 1.0f};  ///< Vertex color (RGBA)

  std::array<u16, kMaxNumVertWeight> bones{0};  ///< Bone indices

  std::array<float, kMaxNumVertWeight> weights{1.0f};  ///< Bone weights. This array
                                                       ///< _must_ contain at least 1 positive weight.

  /**
   * @brief Transforms the vertex using a transformation matrix.
   *
   * This method applies the specified transformation matrix to positions and
   * normals.
   *
   * @param transform The transformation matrix to apply to the vertex.
   */
  void Transform(const glm::mat4& transform);

  /**
   * @brief Checks if the alpha component of the vertex color is less than ~1.0.
   * @return True if the vertex is transparent, false otherwise.
   */
  bool HasAlpha() const;

  /**
   * @brief Compares this vertex with another one for approximate equality.
   * @param rhs The vertex to compare against.
   * @return True if the vertices are approximately equal, false otherwise.
   */
  bool IsApproxEqual(const SVertex& rhs) const;

  /**
   * @brief Normalizes the weights of the vertex so they add up to 1.0.
   */
  void NormalizeWeights();

  /**
   * @brief Quantizes the weights to the specified number of steps.
   * @param steps The number of quantization steps (default is 128).
   */
  void QuantWeights(unsigned int steps = 128);

  /**
   * @brief Sorts vertex weights by influence, placing the highest values first.
   */
  void SortWeights();

  /**
   * @brief Limits the number of bones a vertex refers to by removing the least
   * influential weights.
   *
   * @param max_weights The maximum number of weights to retain.
   */
  void LimitWeights(unsigned int max_weights = 3);

  /**
   * @brief Resets the vertex bone indices and weights to their default value.
   */
  void ResetWeights();

  /**
   * @brief Resets the vertex normal to its default value.
   */
  void ResetNormals();

  /**
   * @brief Resets the texture coordinates to the default value.
   */
  void ResetUVs();

  /**
   * @brief Resets the vertex color to its default value.
   */
  void ResetColors();

  /**
   * @brief Multiplication operator for transforming the vertex by a
   * matrix.
   *
   * This operator allows for the correct transformation of a vertex's position
   * and normal vector using a 4x4 SRT matrix.
   *
   * @param transform The transformation matrix.
   * @param vertex The vertex to transform.
   * @return The transformed vertex.
   */
  friend SVertex operator*(const glm::mat4& transform, SVertex vertex);

  friend SVertex operator*(const SVertex& vertex, float scalar);
};

/**
 * @brief Represents a triangle defined by three vertices.
 *
 * This structure represents a basic triangle defined by three vertices. Its
 * purpose is to simplify the conversion process between various triangle
 * formats such as triangle lists, strips, or fans.
 */
struct STriangle {
  std::array<SVertex, 3> vertices;  ///< The vertices that define the triangle.

  /**
   * @brief Accesses a vertex of the triangle by index.
   *
   * @param index The index of the vertex to access (0, 1, or 2).
   * @return A constant reference to the specified vertex.
   */
  const SVertex& operator[](std::size_t index) const;

  /**
   * @brief Accesses a vertex of the triangle by index.
   *
   * @param index The index of the vertex to access (0, 1, or 2).
   * @return A reference to the specified vertex.
   */
  SVertex& operator[](std::size_t index);

  /**
   * @brief Checks if the triangle has no area.
   *
   * A triangle is considered degenerate by this function if its area is equal
   * to 0.
   *
   * @return True if the area of the triangle is zero.
   *
   */
  bool IsDegenerate() const;

  /**
   * @brief Changes the winding order of the triangle.
   *
   */
  void ReverseWindingOrder();
};

/**
 * @brief Represents a simple mesh.
 *
 * This structure defines a simple mesh representation.
 * It acts as an intermediary between common exchange formats (such as
 * glTF and FBX), and in-game formats.
 *
 * @note All vertices must include at least 1 weight
 * since it's essentially required by the game's model format.
 *
 * @see nnl::model::Mesh
 * @see nnl::model::Convert
 */
struct SMesh {
  std::string name;  ///< An optional name for the mesh.

  std::vector<u32> indices;  ///< The vertex indices that define the triangles

  std::vector<SVertex> vertices;  ///< The vertex data for the mesh.

  std::size_t material_id = 0;  ///< The ID of the material associated with the mesh.

  std::map<std::size_t, std::vector<std::size_t>> material_variants;  ///< This map associates replacement material IDs
                                                                      ///< with the variants they are used in.
                                                                      ///< @see nnl::SModel::material_variants
                                                                      ///< @see nnl::model::TextureSwap

  std::size_t mesh_group = 0;  ///< Indicates the group to which this mesh belongs. It may be used
                               ///< for organizing meshes or animating their visibility.

  bool uses_uv = false;      ///< Indicates whether the mesh uses texture coordinates.
  bool uses_normal = false;  ///< Indicates whether the mesh uses vertex normals.
  bool uses_color = false;   ///< Indicates whether the mesh uses vertex colors.

  SValue extras;  ///< Any additional data for custom use.

  /**
   * @brief Creates a mesh from a triangle list.
   *
   *
   * @param triangles The triangles to construct the mesh from.
   */
  [[nodiscard]] static SMesh FromTriangles(const std::vector<STriangle>& triangles);

  /**
   * @brief Joins another mesh into this mesh.
   *
   * This method combines the vertex and index data of the provided mesh with
   * this mesh.
   *
   * @param smesh The mesh to join with this mesh.
   */
  void Join(const SMesh& smesh);

  /**
   * @brief Finds the center of the mesh.
   *
   * This method calculates the center point of the mesh by averaging the
   * positions of all vertices.
   *
   * @return A glm::vec3 representing the center of the mesh.
   */
  glm::vec3 CalculateCenter() const;

  /**
   * @brief Reverses the winding order of the mesh's triangles.
   *
   * This method changes the order of the vertices in each triangle, which
   * affects the front-facing direction for rendering.
   */
  void ReverseWindingOrder();

  /**
   * @brief Transforms the mesh using a transformation matrix.
   *
   * This method applies the specified transformation matrix to all vertices in
   * the mesh, modifying their positions and normals.
   *
   * @param transform The transformation matrix to apply to the mesh.
   */
  void Transform(const glm::mat4& transform);
  /**
   * @brief Transforms the UV coordinates of the mesh using a transformation
   * matrix.
   *
   * This method applies the specified SRT matrix to the UV coordinates
   * of all vertices in the mesh.
   *
   * @param transform The transformation matrix to apply.
   */
  void TransformUV(const glm::mat3& transform);

  /**
   * @brief Removes duplicate vertices from the mesh.
   *
   * This method removes duplicate vertices and updates the indices.
   *
   * @note Vertices are considered duplicates if their attributes are _nearly_ identical.
   */
  void RemoveDuplicateVertices();

  /**
   * @brief Removes duplicate or unused vertices from the vertex data.

   * @param indices A vector of indices referencing the vertices.
   * @param vertices A vector of vertices.
   * @return A pair containing the unique vertices and updated indices.
   *
   * @note Vertices are considered duplicates if their attributes are _nearly_ identical.
   */
  static std::pair<std::vector<u32>, std::vector<SVertex>> RemoveDuplicateVertices(
      const std::vector<u32>& indices, const std::vector<SVertex>& vertices);

  /**
   * @brief Generates smooth normals for the mesh.
   *
   * This method calculates smooth normals for smooth shading. It also sets
   * uses_normal to true.
   */
  void GenerateSmoothNormals();

  /**
   * @brief Generates flat normals for the mesh.
   *
   * This method calculates flat normals and duplicates vertex data to achieve
   * faceted appearance. It also sets uses_normal to true.
   */
  void GenerateFlatNormals();

  /**
   * @brief Finds the minimum and maximum points of the mesh.
   *
   * This method calculates the minimum and maximum xyz coordinates of the
   * vertices in the mesh.
   *
   * @return A pair of glm::vec3 representing the minimum and maximum xyz
   * points.
   */
  std::pair<glm::vec3, glm::vec3> FindMinMax() const;

  /**
   * @brief Finds the minimum and maximum UV coordinates of the mesh.
   *
   * This method calculates the minimum and maximum UV coordinates of the
   * vertices in the mesh.
   *
   * @return A pair of glm::vec2 representing the minimum and maximum UV
   * coordinates of the mesh.
   */
  std::pair<glm::vec2, glm::vec2> FindMinMaxUV() const;

  /**
   * @brief Checks if the mesh is influenced by at least 2 bones.
   *
   * @return True if the mesh is affected by at least 2 bones.
   */
  bool IsAffectedByFewBones() const;

  /**
   * @brief Checks if the mesh has any transparent vertex colors.
   *
   * @return True if the mesh is transparent.
   */
  bool HasAlphaVertex() const;

  /**
   * @brief Normalizes the weights of the vertices.
   *
   * This method adjusts the vertex weights so that they sum to 1.0f.
   */
  void NormalizeWeights();

  /**
   * @brief Normalizes the normals of the vertices.
   *
   * This method adjusts the vertex normals so that their length is 1.0f.
   */
  void NormalizeNormals();

  /**
   * @brief Quantizes the vertex weights to a specified number of steps.
   *
   * This method reduces the precision of the vertex weights by quantizing them
   * to a specified number of discrete steps, making sure they still add up
   * to 1.0. This may be used for further conversion of weights to fixed-point
   * integers.
   *
   * @param steps The number of quantization steps.
   */
  void QuantWeights(unsigned int steps = 128);

  /**
   * @brief Sorts vertex weights by influence, placing the highest values first.
   */
  void SortWeights();

  /**
   * @brief Limits the number of weights per vertex.
   *
   * This method restricts the number of bone weights that can influence each
   * vertex to a specified maximum.
   *
   * @param max_weights The maximum number of weights per vertex.
   */
  void LimitWeightsPerVertex(unsigned int max_weights = 3);

  /**
   * @brief Limits the number of unique bone influences per triangle.
   *
   * This method restricts the number of unique bones that influence
   * vertices of a triangle to a specified maximum.
   *
   * @param max_weights The maximum number of weights per triangle.
   * @see nnl::model::kMaxNumBonePerPrim
   */
  void LimitWeightsPerTriangle(unsigned int max_weights = 8);

  /**
   * @brief Removes all weights.
   */
  void ResetWeights();

  /**
   * @brief Resets all vertex normals to the default value.
   */
  void ResetNormals();

  /**
   * @brief Resets all texture coordinates to the default value.
   */
  void ResetUVs();

  /**
   * @brief Resets all vertex colors to the default value.
   */
  void ResetColors();
};

/**
 * @brief Represents a bone/joint in a skeletal hierarchy of a 3D model.
 *
 * This structure represents a bone and its children within a skeletal
 * hierarchy, with their transformations defined in relation to their parent
 * bones.
 *
 * @see nnl::SSkeleton
 */
struct SBone {
  std::string name;  ///< An optional name for the bone.

  glm::vec3 scale{1.0f, 1.0f, 1.0f};  ///< The local scale of the bone.

  glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};  ///< The local rotation of the bone.

  glm::vec3 translation{0.0f, 0.0f, 0.0f};  ///< The local translation of the bone.

  glm::mat4 inverse{1.0f};  ///< The inverse bind matrix of the bone.

  std::vector<SBone> children;  ///< The children of this bone.
  /**
   * @brief Calculates the local transformation matrix of the bone from the SRT
   * properties.
   *
   * @return The local transformation matrix.
   */
  glm::mat4 GetTransform() const;
  /**
   * @brief Sets the local SRT properties of the bone.
   *
   * This method decomposes the matrix and sets the local SRT properties.
   *
   * @param transform The local transformation matrix.
   *
   * @note The matrix must be decomposable and invertible.
   */
  void SetTransform(const glm::mat4& transform);
};

/**
 * @brief Represents the skeleton of a 3D model.
 *
 * @note The skeleton should contain at least 1 root bone.
 */
struct SSkeleton {
  std::vector<SBone> roots;  ///< This vector contains all the top-level bones
                             ///< that make up the skeleton's hierarchy.
  /**
   * @brief Retrieves the global matrices.
   *
   * This function computes accumulated transformations for each bone at
   * their location in the hierarchy.
   *
   * @return A vector of glm::mat4 representing the global transformations.
   */
  std::vector<glm::mat4> GetGlobalMatrices() const;

  /**
   * @brief Retrieves the skinning matrices for transforming vertices in model
   * space.
   *
   * This function calculates the global matrices and combines them
   * with the inverse matrices to obtain the skinning transformations, which can
   * be further weighted to determine the appropriate transformations for
   * individual vertices in model space.
   *
   * @return A vector of glm::mat4.
   */
  std::vector<glm::mat4> GetSkinningMatrices() const;

  /**
   * @brief Updates the inverse matrices from local bone transformations.
   *
   * This function recalculates the inverse matrices for all bones,
   * ensuring they are consistent with the current local transformations.
   */
  void UpdateInverseMatrices();

  /**
   * @brief Retrieves a flat array of references to the bones of the skeleton.
   *
   * This function traverses the bone hierarchy using pre-order traversal
   * and collects references to each bone.
   *
   * @return A vector of std::reference_wrapper<const SBone>.
   */
  std::vector<std::reference_wrapper<const SBone>> GetBoneRefs() const;

  /**
   * @copydoc GetBoneRefs.
   */
  std::vector<std::reference_wrapper<SBone>> GetBoneRefs();
};

/**
 * @brief Enumeration of blending modes for materials.
 *
 *
 * @see nnl::SMaterial
 */
enum class NNL_FLAG_ENUM SBlendMode : u8 {
  kOpaque = 0b00000,  ///< Completely opaque.
  kAlpha = 0b00001,   ///< Standard alpha blending.
                      ///< @note The alpha blending is _always_ performed in the games.
                      ///< When converted to an in-game Material, this flag
                      ///< additionally disables depth writing and defers
                      ///< rendering to ensure proper blending.
  kClip = 0b00010,    ///< Alpha clipping mode; fragments below
                      ///< a fixed threshold of 0.5 are discarded.
  kAdd = 0b00100,     ///< Additive blending; colors are added to the background.
  kSub = 0b01000      ///< Subtractive blending; colors are subtracted from the
                      ///< background.
};

NNL_DEFINE_ENUM_OPERATORS(SBlendMode);

/**
 * @brief Enumeration of projection modes for textures.
 *
 * @see nnl::SMaterial
 */
enum class STextureProjection {
  kUV = 0,          ///< Standard UV mapping.
  kMatrix = 1,      ///< Matrix-based projection.
  kEnvironment = 2  ///< Environment mapping projection.
};

/**
 * @brief Represents a simple material with various properties for 3D rendering.
 *
 * This structure contains properties defining the appearance of a material that
 * is typical for the Phong reflection model.
 *
 * @see nnl::model::Material
 *
 * @note Color components must be in the range [0.0, 1.0]
 */
struct SMaterial {
  std::string name;  ///< An optional name for the material.

  glm::vec3 specular{0.0f};  ///< The specular color of the material.
  glm::vec3 diffuse{1.0f};   ///< The diffuse color of the material.
  glm::vec3 ambient{0.5f};   ///< The ambient color of the material.
  glm::vec3 emissive{0.0f};  ///< The emissive color of the material.

  float opacity = 1.0f;  ///< The opacity of the material. [0.0, 1.0]

  float specular_power = 50.0f;  ///< This value determines the intensity and size of specular
                                 ///< highlights. @note In the games this component is typically invisible
                                 ///< @see nnl::lit::Lit

  int texture_id = -1;  ///< The ID of the texture associated with the material. -1 indicates
                        ///< no texture. @see SAsset3D::textures

  bool lit = false;  ///< Indicates whether the material also uses the diffuse,
                     ///< specular, and emissive components in addition to the static
                     ///< ambient color, a texture, and vertex colors.

  u16 vfx_group_id = 0;  ///< The visual effects group of the material.

  SBlendMode alpha_mode = SBlendMode::kOpaque;  ///< Controls how the material is blended with
                                                ///< previously drawn fragments

  STextureProjection projection_mode = STextureProjection::kUV;  ///< Controls how textures are projected onto
                                                                 ///< meshes.

  SValue extras;  ///< Custom additional properties for the
                  ///< material.

  /**
   * @brief Compares two materials for equality.
   *
   * @param rhs The other SMaterial instance to compare against.
   * @return True if both materials are equal.
   */
  bool operator==(const SMaterial& rhs) const;
};

/**
 * @brief Represents a node attached to a bone.
 *
 * @see nnl::model::Attachment
 */
struct SAttachment {
  std::string name = "";  ///< An optional name for the attachment

  std::size_t id = 0;    ///< The id of the struct
  std::size_t bone = 0;  ///< The parent bone

  glm::vec3 scale{1.0f, 1.0f, 1.0f};           ///< Scale transformation.
  glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};  ///< Rotation transformation.
  glm::vec3 translation{0.0f, 0.0f, 0.0f};     ///< Translation transformation.

  glm::mat4 GetTransform() const;

  void SetTransform(const glm::mat4& transform);
};

/**
 * @brief Represents a simple model.
 *
 * This structure defines essential components of a 3D model.
 * It acts as an intermediary between common exchange formats (such as
 * glTF and FBX) and game-specific formats, supporting features that exist (in
 * one way or another) in different formats.
 *
 * @note This struct represents a part of a complete 3D asset.
 *
 * @see nnl::model::Model
 * @see nnl::model::Convert
 * @see nnl::SAsset3D
 */

struct SModel {
  SSkeleton skeleton;  ///< The skeleton associated with the model. It must
                       ///< contain at least 1 bone.

  std::vector<SAttachment> attachments;  ///< A vector of attachments associated with the model.

  std::vector<SMesh> meshes;  ///< A vector of meshes that make up the model.

  std::vector<SMaterial> materials;  ///< A vector of materials used by the meshes.

  std::vector<std::string> material_variants;  ///< Represents material variants similar to those
                                               ///< found in the glTF format. When a specific variant
                                               ///< is active, the material of a mesh changes to the
                                               ///< corresponding material specified in the mesh's
                                               ///< variant mapping.
                                               ///< @see nnl::SMesh

  std::vector<SUVAnimation> uv_animations;  ///< A vector of texture coordinate animations for
                                            ///< textured materials
  /**
   * @brief Finds the center of the model.
   *
   * This method calculates the center point of the mesh by averaging the
   * positions of all mesh vertices.
   *
   * @return A glm::vec3 representing the center of the model.
   */
  glm::vec3 CalculateCenter() const;

  /**
   * @brief Bakes bind shape transforms from the inverse matrices into the
   * model.
   *
   * This method applies "bind shape matrices" that may be contained in inverse
   * bind matrices to mesh vertices. If some matrices are non-invertible, it
   * returns false.
   *
   * Inverse matrices often include a scaling matrix
   * to revert the model to its original proportions after
   * normalization to the range of [-1.0, 1.0].
   *
   * @return bool true on success
   *
   * @note This method assumes that local bone transforms represent the bind
   * pose.
   */
  bool TryBakeBindShape();

  /**
   * @brief Transforms the model using a transformation matrix.
   *
   * This method applies the specified transformation matrix to all vertices in
   * the model, modifying their positions and normals.
   *
   * @param matrix The transformation matrix to apply.
   */
  void Transform(const glm::mat4& matrix);

  /**
   * @brief Transforms the UV coordinates of the model using a transformation
   * matrix.
   *
   * This method applies the specified SRT matrix to the UV coordinates
   * of all vertices in the model.
   *
   * @param matrix The transformation matrix to apply.
   */
  void TransformUV(const glm::mat3& matrix);

  /**
   * @brief Removes duplicate materials from the model.
   *
   * This function removes any materials that are identical,
   * which may improve the performance.
   */
  void RemoveDuplicateMaterials();

  /**
   * @brief Generates smooth normals for the meshes.
   *
   * This method calculates smooth normals for smooth shading.
   */
  void GenerateSmoothNormals();

  /**
   * @brief Generates flat normals for the meshes.
   *
   * This method calculates face normals and duplicates vertex data to achive
   * faceted appearance.
   */
  void GenerateFlatNormals();

  /**
   * @brief Finds the minimum and maximum points of the model.
   *
   * This method finds the minimum and maximum xyz coordinates of the
   * vertices in the model.
   *
   * @return A pair of glm::vec3 representing the minimum and maximum xyz
   * points.
   */
  std::pair<glm::vec3, glm::vec3> FindMinMax() const;

  /**
   * @brief Normalizes the model to fit within the range of -1.0 to 1.0.
   *
   * This function changes the model's vertices to ensure they fit within
   * the range of -1.0 to 1.0. It also modifies the inverse matrices so they undo the
   * scaling. This normalization may be useful for converting positions to
   * fixed-point integers.
   *
   * @return glm::mat4 The scale matrix that was applied to the inverse
   * matrices.
   */
  glm::mat4 Normalize();

  /**
   * @brief Normalizes the UV coordinates of the model.
   *
   * This function rescales the UV coordinates to ensure they fit within the
   * range of 0.0 to 1.0. Each mesh group is normalized independently.
   * This normalization may be useful for converting texture coordinates
   * to fixed-point integers.
   *
   * @return A collection of pairs for each mesh group, containing the scale
   * and offset transformations needed to restore UV coordinates to their
   * original values.
   * @see nnl::model::MeshGroup
   */
  std::vector<std::pair<glm::vec2, glm::vec2>> NormalizeUV();

  /**
   * @brief Normalizes the weights of the vertices.
   *
   * This method adjusts the vertex weights so that they sum to 1.0f.
   */
  void NormalizeWeights();

  /**
   * @brief Normalizes the normals of the vertices.
   *
   * This method adjusts the vertex normals so that their length is 1.0f.
   */
  void NormalizeNormals();

  /**
   * @brief Quantizes the vertex weights to a specified number of steps.
   *
   * This method reduces the precision of the vertex weights by quantizing them
   * to a specified number of discrete steps, making sure they still add up
   * to 1.0. This may be used for further conversion of weights to fixed-point
   * integers.
   *
   * @param steps The number of quantization steps.
   */
  void QuantWeights(unsigned int steps = 128);

  /**
   * @brief Sorts vertex weights by influence, placing the highest values first.
   */
  void SortWeights();

  /**
   * @brief Limits the number of weights per vertex.
   *
   * This method restricts the number of bone weights that can influence each
   * vertex to a specified maximum.
   *
   * @param max_weights The maximum number of weights per vertex.
   */
  void LimitWeightsPerVertex(unsigned int max_weights = 3);

  /**
   * @brief Limits the number of weights per triangle.
   *
   * This method restricts the number of bone weights that can influence
   * vertices of a triangle to a specified maximum.
   *
   * @param max_weights The maximum number of weights per triangle.
   * @see nnl::model::kMaxNumBonePerPrim
   */
  void LimitWeightsPerTriangle(unsigned int max_weights = 8);

  /**
   * @brief Removes all weights.
   */
  void ResetWeights();

  /**
   * @brief Resets all vertex normals to the default value.
   */
  void ResetNormals();

  /**
   * @brief Resets all texture coordinates to the default value.
   */
  void ResetUVs();

  /**
   * @brief Resets all vertex colors to the default value.
   */
  void ResetColors();
};
/** @} */
}  // namespace nnl
