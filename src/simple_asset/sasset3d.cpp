
#include "NNL/simple_asset/sasset3d.hpp"

#include "NNL/utility/math.hpp"
namespace nnl {

glm::mat4 SLight::GetTransform(glm::vec3 base) const {
  glm::vec3 direction = glm::normalize(this->direction);

  glm::mat4 transform{1.0f};

  glm::vec3 axis = glm::normalize(glm::cross(base, direction));

  // The direction is default
  if (utl::math::IsApproxEqual(glm::vec3(0), axis)) {
    return transform;
  }

  float dot = glm::dot(base, direction);
  // Transform from base to direction
  transform = glm::rotate(transform, glm::acos(dot), axis);

  return transform;
}

void SLight::SetDirection(const glm::mat4& transform, glm::vec3 base) {
  auto [scale, rotation, translation] = utl::math::Decompose(transform);

  this->direction = glm::normalize(glm::toMat3(rotation) * base);
}

glm::mat4 SPosition::GetTransform() const { return utl::math::Compose(scale, rotation, translation); }

void SPosition::SetTransform(const glm::mat4& transform) {
  std::tie(scale, rotation, translation) = utl::math::Decompose(transform);
}

void SAsset3D::Scale(float scale) {
  NNL_EXPECTS(scale > 0.0f);
  auto scale_mat = glm::scale(glm::mat4(1.0f), glm::vec3(scale));

  model.Transform(scale_mat);

  auto bone_refs = model.skeleton.GetBoneRefs();

  for (std::size_t i = 0; i < bone_refs.size(); i++) {
    auto& bone = bone_refs[i].get();

    bone.translation = scale_mat * glm::vec4(bone.translation, 1.0f);
    bone.inverse[3] = scale_mat * bone.inverse[3];
  }

  for (auto& sanimation : animations) {
    for (auto& channel : sanimation.bone_channels) {
      for (auto& t : channel.translation_keys) t.value = scale_mat * glm::vec4(t.value, 1.0f);
    }
  }

  for (auto& sattach : model.attachments) {
    sattach.translation = scale_mat * glm::vec4(sattach.translation, 1.0f);
  }

  for (auto& pos : positions) {
    pos.translation = scale_mat * glm::vec4(pos.translation, 1.0f);
  }
}

bool SAsset3D::TrySimplifySkeleton() {
  if (!model.TryBakeBindShape()) {
    return false;
  }

  std::function<void(SBone&)> CalculateNewLocal;

  // Recalculates local bone matrices from inverse bind matrices
  CalculateNewLocal = [&CalculateNewLocal](SBone& parent) {
    for (auto& child : parent.children) {
      child.SetTransform(parent.inverse * utl::math::Inverse(child.inverse));
      CalculateNewLocal(child);
    }
  };

  auto new_skeleton = model.skeleton;

  // Create new skeleton
  {
    auto new_bone_refs = new_skeleton.GetBoneRefs();

    for (auto& bone_ref : new_bone_refs) {
      auto& bone = bone_ref.get();
      bone.scale = glm::vec3(1.0f);
      bone.translation = glm::vec3(0.0f);
      bone.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
      glm::vec3 translation = utl::math::Inverse(bone.inverse) * glm::vec4(0, 0, 0, 1.0f);
      bone.inverse = utl::math::Inverse(glm::translate(glm::mat4(1.0f), translation));
    }

    for (auto& root : new_skeleton.roots) {
      root.SetTransform(utl::math::Inverse(root.inverse));
      CalculateNewLocal(root);
    }
  }
  // Recalculate animations
  for (auto& sanimation : animations) {
    sanimation.Bake();

    for (std::size_t k = 0; k < sanimation.duration; k++) {
      // Copy the original source skeleton
      // (used for convenience)
      auto tmp_skeleton = model.skeleton;
      auto tmp_bone_refs = tmp_skeleton.GetBoneRefs();

      // Set local transforms using the animation keyframes
      // (to calculate skinning matrices later)
      for (std::size_t i = 0; i < sanimation.bone_channels.size(); i++) {
        auto& channel = sanimation.bone_channels[i];

        auto& scale = channel.scale_keys.at(k).value;
        auto& rotation = channel.rotation_keys.at(k).value;
        auto& translation = channel.translation_keys.at(k).value;

        auto& tmp_bone = tmp_bone_refs.at(i).get();

        tmp_bone.scale = scale;
        tmp_bone.rotation = rotation;
        tmp_bone.translation = translation;
      }

      // The skinning matrix should stay the same no matter what the skeleton is:

      // unknown global transform * known new inverse = known skinning matrix

      auto skinning_matrices = tmp_skeleton.GetSkinningMatrices();
      auto new_bone_refs = new_skeleton.GetBoneRefs();

      for (std::size_t i = 0; i < new_bone_refs.size(); i++) {
        auto& new_bone_ref = new_bone_refs[i];
        auto& tmp_bone_ref = tmp_bone_refs.at(i);
        auto new_global_matrix = skinning_matrices.at(i) * utl::math::Inverse(new_bone_ref.get().inverse);
        // Set new inverse to a tmp skeleton to properly calculate new local
        // transforms for animations (from global transform)
        tmp_bone_ref.get().inverse = utl::math::InverseSafe(new_global_matrix);
      }

      // Calculate new local transforms for animations
      for (auto& tmp_root : tmp_skeleton.roots) {
        tmp_root.SetTransform(utl::math::Inverse(tmp_root.inverse));
        CalculateNewLocal(tmp_root);
      }

      // Update animations to reflect the changes
      for (std::size_t i = 0; i < sanimation.bone_channels.size(); i++) {
        auto& channel = sanimation.bone_channels[i];
        const auto& bone = tmp_bone_refs.at(i).get();

        auto& scale_key = channel.scale_keys.at(k);
        auto& rotation_key = channel.rotation_keys.at(k);
        auto& translation_key = channel.translation_keys.at(k);

        scale_key.value = bone.scale;
        rotation_key.value = bone.rotation;
        translation_key.value = bone.translation;

        assert(utl::math::IsFinite(scale_key.value));
        assert(utl::math::IsFinite(rotation_key.value));
        assert(utl::math::IsFinite(translation_key.value));
      }
    }
  }

  model.skeleton = new_skeleton;

  return true;
}

void SAsset3D::SortForBlending(glm::vec3 reference_point) {
  std::map<std::string, bool> is_mesh_transparent;
  std::map<std::string, float> distance;

  std::vector<unsigned char> is_texture_transparent(textures.size());

  for (std::size_t i = 0; i < textures.size(); i++) {
    const auto& stexture = textures.at(i);
    is_texture_transparent.at(i) = stexture.HasAlpha();
  }

  auto IsTransparent = [this, &is_texture_transparent](const SMesh& smesh) -> bool {
    const auto& smat = model.materials.at(smesh.material_id);

    if ((smat.alpha_mode & SBlendMode::kClip) != SBlendMode::kOpaque) return false;

    if (smat.opacity < NNL_ALPHA_OPAQ_F || smat.alpha_mode != SBlendMode::kOpaque ||
        smat.projection_mode != STextureProjection::kUV || smesh.HasAlphaVertex())
      return true;  // Projection modes other than kUV disable depth write

    if (smat.texture_id != -1 && is_texture_transparent.at(smat.texture_id)) return true;

    return false;
  };

  for (const auto& smesh : model.meshes) {
    NNL_EXPECTS_DBG(!smesh.name.empty());
    is_mesh_transparent.insert({smesh.name, IsTransparent(smesh)});
  }

  auto DistanceFarthest = [reference_point](const SMesh& mesh) -> float {
    float length_far = 0.0f;

    for (auto& vertex : mesh.vertices) {
      float l = glm::length2(vertex.position - reference_point);

      if (l > length_far) length_far = l;
    }

    return length_far;
  };

  for (const auto& smesh : model.meshes) distance.insert({smesh.name, DistanceFarthest(smesh)});

  std::sort(std::begin(model.meshes), std::end(model.meshes),
            [&is_mesh_transparent, &distance](const SMesh& a, const SMesh& b) {
              bool a1 = is_mesh_transparent.at(a.name);
              bool a2 = is_mesh_transparent.at(b.name);

              auto distance_1 = distance.at(a.name);
              auto distance_2 = distance.at(b.name);

              return (a2 > a1) ||                                                 // opaque first
                     ((a2 == a1 && a1 == false) && (distance_1 < distance_2)) ||  // both opaque: nearest first
                     ((a2 == a1 && a1 == true) && (distance_1 > distance_2));     // both transparent: farthest first
            });
}

}  // namespace nnl
