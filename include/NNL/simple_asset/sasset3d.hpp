/**
 * @file sasset3d.hpp
 * @brief Provides data structures for representing essential components of a 3D asset.
 *
 * This file provides data structures and functions for managing
 * essential parts of 3D assets. These structures enable importing and
 * exporting data between common exchange formats (such as glTF or FBX) and
 * in-game formats.
 *
 * @see nnl::SAsset3D
 * @see nnl::SModel
 * @see nnl::STexture
 * @see nnl::SAnimation
 * @see nnl::SVisibilityAnimation
 */
#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "NNL/simple_asset/sanimation.hpp"
#include "NNL/simple_asset/smodel.hpp"
#include "NNL/simple_asset/stexture.hpp"

namespace nnl {

/**
 * \defgroup Simple3DAsset Simple 3D Asset
 * @ingroup SimpleAssets
 * @copydoc sasset3d.hpp
 * @{
 */
/**
 * @brief Represents an empty node with a transform.
 *
 * @see nnl::posd::Position
 * @see nnl::minimap::Convert
 * @see nnl::SAsset3D
 */
struct SPosition {
  std::string name;  ///< An optional name for the struct.

  std::size_t id = 0;  ///< Unique identifier for the node.

  glm::vec3 scale{1.0f, 1.0f, 1.0f};           ///< Scale transformation.
  glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};  ///< Rotation transformation.
  glm::vec3 translation{0.0f, 0.0f, 0.0f};     ///< Translation transformation.

  SValue extras;  ///< Any additional data for custom use.

  /**
   * @brief Calculates the local transformation matrix from the SRT
   * properties.
   *
   * @return The local transformation matrix.
   */
  glm::mat4 GetTransform() const;

  /**
   * @brief Sets the local SRT properties.
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
 * @brief Represents a light source in a 3D scene.
 *
 * @see nnl::lit::Light
 * @see nnl::lit::Convert
 * @see nnl::SAsset3D
 */
struct SLight {
  std::string name;                           ///< An optional name for the light.
  glm::vec3 direction = {0.0f, -1.0f, 0.0f};  ///< Light direction **from** the source.
  glm::vec3 color = {1.0f, 1.0f, 1.0f};       ///< Color of the light.

  SValue extras;  ///< Any additional data for custom use.

  /**
   * @brief Computes the transformation matrix for the light.
   *
   * Computes the matrix that transforms the base light direction to the
   * direction specified in this struct.
   *
   * @param base The default direction of the light (may differ between formats).
   * @return The transformation matrix for the light.
   */
  glm::mat4 GetTransform(glm::vec3 base = glm::vec3(0, 0, -1.0f)) const;

  /**
   * @brief Sets the direction of the light based on a transformation matrix.
   *
   * @param transform The transformation matrix to apply to the base.
   * @param base The default direction of the light (may differ between formats).
   */
  void SetDirection(const glm::mat4& transform, glm::vec3 base = glm::vec3(0, 0, -1.0f));
};

/**
 * @brief Represents a 3D asset composed of various components.
 *
 * This structure serves as the primary container for various components of a 3D
 * asset, acting as an intermediary representation for conversion between common
 * exchange formats (such as glTF and FBX) and in-game formats.
 *
 * @see nnl::SAsset3D::Import
 * @see nnl::SAsset3D::ExportGLB
 * @see nnl::asset::Asset3D
 */
struct SAsset3D {
  std::string name;                                         ///< An optional name for the asset.
  SModel model;                                             ///< The 3D model.
  std::vector<STexture> textures;                           ///< Textures used by the materials of the model.
  std::vector<SAnimation> animations;                       ///< Skeletal animations.
  std::vector<SVisibilityAnimation> visibility_animations;  ///< Visibility animations for mesh groups of the model.
  std::vector<SLight> lights;                               ///< Light sources.
  std::vector<SPosition> positions;                         ///< Spawn positions.
  SValue extras;                                            ///< Any additional data for custom use.
  /**
   * @brief Constructs an asset from a glTF file located at the specified path.
   *
   * This method loads data from a glTF asset found at the given path.
   *
   * @param path The file path to the glTF asset.
   * @param flip Indicates whether to vertically flip textures during loading.
   * @param decode_images If false, STexture objects will retain the original image files,
   * with the height property indicating the size of these files.
   */
  [[nodiscard]] static SAsset3D Import(const std::filesystem::path& path, bool flip = true, bool decode_images = true);
  /**
   * @brief Constructs an asset from a glTF file located in the buffer.
   *
   * This method loads data from a glTF asset found in the given buffer.
   *
   * @param buffer The buffer containing a glTF asset.
   * @param base_path The path for loading external data buffers or images.
   * @param flip Indicates whether to vertically flip textures during loading.
   * @param decode_images If false, STexture objects will retain the original image files,
   * with the height property indicating the size of these files.
   */
  [[nodiscard]] static SAsset3D Import(BufferView buffer, const std::filesystem::path& base_path = {}, bool flip = true,
                                       bool decode_images = true);
  /**
   * @brief Exports the asset data to a GLB file.
   *
   * This method saves the asset data to the specified file path in the glTF format.
   *
   * @param path The file path where the glTF asset will be exported.
   * @param flip Indicates whether to vertically flip textures during export.

   * @param pack_textures If true, the textures will be packed into the exported
   * file.
   */
  void ExportGLB(const std::filesystem::path& path, bool flip = true, bool pack_textures = true) const;

  /**
   * @brief Exports the asset data to a GLB file.
   *
   * This method saves the asset data in the glTF format to a buffer.
   *
   * @param flip Indicates whether to vertically flip textures during export.

   */
  [[nodiscard]] Buffer ExportGLB(bool flip = true) const;
  /**
   * @brief Scales the asset by a specified factor.
   *
   * This method applies a uniform scale transformation to all components
   * of the asset, including the model and its skeleton, spawn positions, and
   * animations.
   *
   * @param scale The scaling factor to apply.
   *
   * @note The root bone is not transformed by this method so objects
   * attached to the asset (effects, hitboxes) are not affected.
   */
  void Scale(float scale = 1.0f);

  /**
   * @brief Simplifies the skeleton by retaining only translation transformations.
   *
   * Modifies bones and animations to preserve bone positions while resetting
   * rotation and scale to default values.
   *
   * @return true on success
   */
  bool TrySimplifySkeleton();

  /**
   * @brief Sorts meshes for proper blending and improved
   * performance.
   *
   * Re-orders meshes by distance from the reference point, aiming to achieve correct alpha blending
   * and to minimize overdraw.
   *
   * @param reference_point The position in the world used to calculate the distance.
   *
   * @see nnl::SModel::CalculateCenter
   */
  void SortForBlending(glm::vec3 reference_point);
};
/** @} */

}  // namespace nnl
