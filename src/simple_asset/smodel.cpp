#include "NNL/simple_asset/smodel.hpp"

#include <set>

#include "NNL/utility/math.hpp"
#include "NNL/utility/static_set.hpp"
namespace nnl {

glm::mat4 SAttachment::GetTransform() const { return utl::math::Compose(scale, rotation, translation); }

void SAttachment::SetTransform(const glm::mat4& transform) {
  std::tie(scale, rotation, translation) = utl::math::Decompose(transform);
}

void SVertex::Transform(const glm::mat4& transform) {
  NNL_EXPECTS_DBG(utl::math::IsFinite(transform));
  NNL_EXPECTS_DBG(utl::math::IsFinite(position));
  NNL_EXPECTS_DBG(utl::math::IsFinite(normal));

  position = transform * glm::vec4(position, 1.0f);

  normal = glm::transpose(utl::math::Inverse(glm::mat3(transform))) * normal;

  normal = utl::math::NormalizeSafe(normal);
}

SVertex operator*(const glm::mat4& transform, SVertex vertex) {
  vertex.Transform(transform);
  return vertex;
}

SVertex operator*(const SVertex& vertex, float scalar) {
  glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(scalar));
  return scale * vertex;
}

void SVertex::QuantWeights(unsigned int steps) {
  NNL_EXPECTS_DBG(steps > 0);

  float sum = 0;
  float steps_f = static_cast<float>(steps);
  float max_weight = std::numeric_limits<float>::lowest();

  std::size_t max_id = 0;

  for (std::size_t i = 0; i < weights.size(); i++) {
    float& weight = weights[i];
    weight = std::round(weight * steps_f);
    sum += weight;
    if (weight > max_weight) {
      max_weight = weight;
      max_id = i;
    }
  }

  float remainder = steps_f - sum;

  weights[max_id] += remainder;

  for (auto& weight : weights) {
    weight = weight / steps;
  }
}

void SVertex::LimitWeights(unsigned int max_weights) {
  NNL_EXPECTS_DBG(max_weights > 0 && max_weights <= kMaxNumBoneWeight);

  this->SortWeights();

  for (std::size_t i = max_weights; i < bones.size(); i++) {
    bones[i] = 0;
    weights[i] = 0.0f;
  }

  this->NormalizeWeights();
}

void SVertex::ResetWeights() {
  bones = {0};
  weights = {1.0f};
}

void SVertex::ResetNormals() { normal = {0.0f, 1.0f, 0.0f}; }

void SVertex::ResetUVs() { uv = {0.0f, 0.0f}; }

void SVertex::ResetColors() { color = {1.0f, 1.0f, 1.0f, 1.0f}; }

bool SVertex::HasAlpha() const { return color.a < NNL_ALPHA_OPAQ_F; }

void SVertex::NormalizeWeights() {
  float vertex_weight_sum = 0.0f;

  for (auto& weight : weights) {
    NNL_EXPECTS_DBG(weight >= 0.0f);
    vertex_weight_sum += weight;
  }

  for (auto& weight : weights) weight /= vertex_weight_sum;
}

void SVertex::SortWeights() {
  std::array<std::pair<float, u16>, kMaxNumBoneWeight> combined;

  for (std::size_t i = 0; i < combined.size(); i++) {
    combined[i] = {weights[i], bones[i]};
  }

  std::sort(combined.begin(), combined.end(), [](const auto w0, const auto w1) { return w0.first > w1.first; });

  for (std::size_t i = 0; i < combined.size(); i++) {
    weights[i] = combined[i].first;
    bones[i] = combined[i].second;
  }
}

bool SVertex::IsApproxEqual(const SVertex& rhs) const {
  if (!utl::math::IsApproxEqual(position, rhs.position)) return false;

  if (!utl::math::IsApproxEqual(uv, rhs.uv)) return false;

  if (!utl::math::IsApproxEqual(normal, rhs.normal)) return false;

  if (color != rhs.color) return false;

  // There are few cases where it's the only way to distinguish 2 vertices
  if (bones != rhs.bones) return false;

  return true;
}

const SVertex& STriangle::operator[](std::size_t index) const {
  NNL_EXPECTS_DBG(index < vertices.size());
  return vertices[index];
}

SVertex& STriangle::operator[](std::size_t index) {
  NNL_EXPECTS_DBG(index < vertices.size());
  return vertices[index];
}

bool STriangle::IsDegenerate() const {
  NNL_EXPECTS_DBG(utl::math::IsFinite(vertices[0].position));
  NNL_EXPECTS_DBG(utl::math::IsFinite(vertices[1].position));
  NNL_EXPECTS_DBG(utl::math::IsFinite(vertices[2].position));
  glm::vec3 p1p2 = vertices[1].position - vertices[0].position;
  glm::vec3 p1p3 = vertices[2].position - vertices[0].position;

  float area = glm::length(glm::cross(p1p2, p1p3)) / 2.0f;

  return (area < NNL_EPSILON2);
}

void STriangle::ReverseWindingOrder() {
  auto tmp_vertex = vertices[0];
  vertices[0] = vertices[2];
  vertices[2] = tmp_vertex;
}

inline void hash_combine(std::size_t&) {
  // empty
}

template <typename T, typename... Rest>
inline void hash_combine(std::size_t& seed, const T& v, Rest... rest) {
  std::hash<T> hasher;
  seed ^= hasher(std::round(v * 1000.0f)) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
  hash_combine(seed, rest...);
}

struct SVertexHash {
  std::size_t operator()(const SVertex& v) const noexcept {
    std::size_t seed = 0;
    hash_combine(seed, v.position.x, v.position.y, v.position.z);
    return seed;
  }
};

struct SVertexEq {
  std::size_t operator()(const SVertex& lhs, const SVertex& rhs) const noexcept { return lhs.IsApproxEqual(rhs); }
};

SMesh SMesh::FromTriangles(const std::vector<STriangle>& triangles) {
  SMesh smesh;
  auto& new_indices = smesh.indices;
  new_indices.reserve(triangles.size() * 3);

  auto& new_vertices = smesh.vertices;
  new_vertices.reserve(triangles.size() * 3);

  std::unordered_map<SVertex, u32, SVertexHash, SVertexEq> vertex2Index(triangles.size() * 3);

  for (auto& triangle : triangles) {
    for (std::size_t i = 0; i < 3; i++) {
      SVertex vertex = triangle[i];

      vertex.SortWeights();

      auto itr_vertex = vertex2Index.find(vertex);

      if (itr_vertex == vertex2Index.end()) {
        vertex2Index[vertex] = new_vertices.size();
        new_indices.push_back(new_vertices.size());
        new_vertices.push_back(vertex);
      } else {
        new_indices.push_back(itr_vertex->second);
      }
    }
  }
  NNL_ENSURES_DBG((!new_indices.empty() && !new_vertices.empty()) || triangles.empty());
  NNL_ENSURES_DBG(new_indices.size() % 3 == 0);
  return smesh;
}

void SMesh::Join(const SMesh& smesh) {
  std::size_t new_indices_pos = indices.size();

  indices.resize(indices.size() + smesh.indices.size());

  for (std::size_t i = new_indices_pos, j = 0; i < indices.size(); i++, j++)
    indices[i] = smesh.indices[j] + vertices.size();

  vertices.insert(vertices.end(), smesh.vertices.begin(), smesh.vertices.end());
}

glm::vec3 SMesh::CalculateCenter() const {
  NNL_EXPECTS(!vertices.empty());
  glm::vec3 accum{0};

  for (auto& vertex : vertices) accum += vertex.position;

  return accum / static_cast<float>(vertices.size());
}

void SMesh::ReverseWindingOrder() {
  NNL_EXPECTS(indices.size() % 3 == 0);
  for (std::size_t i = 0; i < indices.size(); i += 3) {
    auto tmp_index = indices[i + 0];
    indices[i + 0] = indices[i + 2];
    indices[i + 2] = tmp_index;
  }
}

bool SMesh::IsAffectedByFewBones() const {
  std::set<u16> unique_ids;
  for (auto& vertex : vertices) {
    for (std::size_t i = 0; i < vertex.bones.size(); i++) {
      if (vertex.weights[i] > 0.0f) unique_ids.insert(vertex.bones[i]);
    }

    if (unique_ids.size() > 1) return true;
  }
  return unique_ids.size() > 1;
}

bool SMesh::HasAlphaVertex() const {
  if (uses_color) {
    for (auto& vertex : vertices) {
      if (vertex.HasAlpha()) return true;
    }
  }
  return false;
}

void SMesh::Transform(const glm::mat4& transform) {
  NNL_EXPECTS_DBG(utl::math::IsFinite(transform));
  for (auto& vertex : vertices) vertex.Transform(transform);
}

void SMesh::TransformUV(const glm::mat3& transform) {
  NNL_EXPECTS_DBG(utl::math::IsFinite(transform));
  for (auto& vertex : vertices) vertex.uv = transform * glm::vec3(vertex.uv, 1.0f);
}

void SMesh::ResetWeights() {
  for (auto& vertex : vertices) {
    vertex.ResetWeights();
  }
}

void SMesh::ResetNormals() {
  for (auto& vertex : vertices) {
    vertex.ResetNormals();
  }
}

void SMesh::ResetUVs() {
  for (auto& vertex : vertices) {
    vertex.ResetUVs();
  }
}

void SMesh::ResetColors() {
  for (auto& vertex : vertices) {
    vertex.ResetColors();
  }
}

void SMesh::NormalizeWeights() {
  for (auto& vertex : vertices) {
    vertex.NormalizeWeights();
  }
}

void SMesh::NormalizeNormals() {
  for (auto& vertex : vertices) {
    vertex.normal = utl::math::NormalizeSafe(vertex.normal);
  }
}

void SMesh::QuantWeights(unsigned int steps) {
  NNL_EXPECTS(steps > 0);
  for (auto& vertex : vertices) {
    vertex.QuantWeights(steps);
  }
}

void SMesh::SortWeights() {
  for (auto& vertex : vertices) {
    vertex.SortWeights();
  }
}

std::pair<glm::vec3, glm::vec3> SMesh::FindMinMax() const {
  float minX = std::numeric_limits<float>::max();
  float maxX = std::numeric_limits<float>::lowest();

  float minY = std::numeric_limits<float>::max();
  float maxY = std::numeric_limits<float>::lowest();

  float minZ = std::numeric_limits<float>::max();
  float maxZ = std::numeric_limits<float>::lowest();

  for (const auto& vertex : vertices) {
    auto& pos = vertex.position;

    if (pos.x > maxX) maxX = pos.x;
    if (pos.x < minX) minX = pos.x;

    if (pos.y > maxY) maxY = pos.y;
    if (pos.y < minY) minY = pos.y;

    if (pos.z > maxZ) maxZ = pos.z;
    if (pos.z < minZ) minZ = pos.z;
  }

  return std::make_pair(glm::vec3(minX, minY, minZ), glm::vec3(maxX, maxY, maxZ));
}

std::pair<glm::vec2, glm::vec2> SMesh::FindMinMaxUV() const {
  float minX = std::numeric_limits<float>::max();
  float maxX = std::numeric_limits<float>::lowest();

  float minY = std::numeric_limits<float>::max();
  float maxY = std::numeric_limits<float>::lowest();

  for (const auto& vertex : vertices) {
    auto& uv = vertex.uv;

    if (uv.x > maxX) maxX = uv.x;
    if (uv.x < minX) minX = uv.x;

    if (uv.y > maxY) maxY = uv.y;
    if (uv.y < minY) minY = uv.y;
  }

  return std::make_pair(glm::vec2(minX, minY), glm::vec2(maxX, maxY));
}

void SMesh::LimitWeightsPerVertex(unsigned int max_weights) {
  NNL_EXPECTS(max_weights > 0 && max_weights <= kMaxNumBoneWeight);
  for (auto& vertex : vertices) {
    vertex.LimitWeights(max_weights);
  }
}

void SMesh::LimitWeightsPerTriangle(unsigned int max_weights) {
  NNL_EXPECTS(max_weights > 0);
  NNL_EXPECTS(indices.size() % 3 == 0);

  for (std::size_t i = 0; i < indices.size(); i += 3) {
    std::array<SVertex*, 3> triangle_vertices{&vertices.at(indices[i]), &vertices.at(indices[i + 1]),
                                              &vertices.at(indices[i + 2])};

    utl::StaticSet<u16, 3 * kMaxNumBoneWeight> unique_bones;
    std::array<std::size_t, 3> vert_num_bones{0};

    // Get unique bones of the primitive
    for (std::size_t k = 0; k < 3; k++) {
      SVertex& vertex = *triangle_vertices[k];

      std::size_t num_bones = 0;

      for (std::size_t l = 0; l < vertex.bones.size(); l++) {
        if (vertex.weights[l] > 0.0f) {
          unique_bones.Insert(vertex.bones[l]);
          num_bones++;
        }
      }

      assert(num_bones > 0);

      vert_num_bones[k] = num_bones;
    }

    if (unique_bones.Size() <= max_weights) continue;

    std::unordered_map<u16, f32> bone_total_weight;

    // Calculate the sum of all weights of a bone in a triangle
    for (auto vertex_ref : triangle_vertices) {
      SVertex& vertex = *vertex_ref;

      for (std::size_t j = 0; j < vertex.bones.size(); j++) {
        if (vertex.weights[j] > 0.0f) {
          bone_total_weight[vertex.bones[j]] += vertex.weights[j];
        }
      }
    }

    // Find the least influential bone id (smallest total)

    while (bone_total_weight.size() > max_weights) {
      u16 least_influential_bone_id = -1;

      float min_total_weight = std::numeric_limits<float>::max();

      for (auto [bone_id, total_weight] : bone_total_weight) {
        if (total_weight < min_total_weight) {
          min_total_weight = total_weight;
          least_influential_bone_id = bone_id;
        }
      }

      assert(least_influential_bone_id != u16(-1));

      bone_total_weight.erase(least_influential_bone_id);

      // Delete this bone from weight data
      for (std::size_t j = 0; j < triangle_vertices.size(); j++) {
        SVertex& vertex = *triangle_vertices[j];

        if (vert_num_bones[j] <= 1) continue;

        auto min_weight_itr = std::find(vertex.bones.begin(), vertex.bones.end(), least_influential_bone_id);

        if (min_weight_itr == vertex.bones.end()) continue;

        auto min_weight_index = std::distance(vertex.bones.begin(), min_weight_itr);

        float min_weight = vertex.weights[min_weight_index];

        vertex.weights[min_weight_index] = 0.0f;
        vertex.bones[min_weight_index] = 0;

        vert_num_bones[j] -= 1;

        float weight_remainder = min_weight / (float)vert_num_bones[j];

        // Normalize
        for (auto& weight : vertex.weights) {
          if (weight > 0.0f) {
            weight += weight_remainder;
          }
        }
      }
    }
  }
}

std::pair<std::vector<u32>, std::vector<SVertex>> SMesh::RemoveDuplicateVertices(const std::vector<u32>& indices,
                                                                                 const std::vector<SVertex>& vertices) {
  std::vector<u32> new_indices;

  new_indices.reserve(indices.size());

  std::vector<SVertex> new_vertices;

  new_vertices.reserve(vertices.size());

  std::unordered_map<SVertex, u32, SVertexHash, SVertexEq> vertex2Index(vertices.size());

  for (std::size_t i = 0; i < indices.size(); i++) {
    SVertex vertex = vertices.at(indices[i]);

    vertex.SortWeights();

    auto itr_vertex = vertex2Index.find(vertex);

    if (itr_vertex == vertex2Index.end()) {
      vertex2Index[vertex] = new_vertices.size();
      new_indices.push_back(new_vertices.size());
      new_vertices.push_back(vertex);
    } else {
      new_indices.push_back(itr_vertex->second);
    }
  }

  return {new_indices, new_vertices};
}

void SMesh::RemoveDuplicateVertices() {
  auto [new_indices, new_vertices] = SMesh::RemoveDuplicateVertices(indices, vertices);
  indices = std::move(new_indices);
  vertices = std::move(new_vertices);
}

void SMesh::GenerateSmoothNormals() {
  NNL_EXPECTS(indices.size() % 3 == 0);

  uses_normal = true;

  std::vector<glm::vec3> normals{vertices.size(), glm::vec3(0.0f)};

  for (std::size_t i = 0; i < indices.size(); i += 3) {
    glm::vec3 p1 = vertices[indices[i]].position;
    glm::vec3 p2 = vertices[indices[i + 1]].position;
    glm::vec3 p3 = vertices[indices[i + 2]].position;

    glm::vec3 a = p3 - p2;
    glm::vec3 b = p1 - p2;

    glm::vec3 cross = glm::cross(a, b);

    glm::vec3 normal{0};

    if (cross != glm::vec3(0)) normal = glm::normalize(cross);

    normals[indices[i]] += normal;
    normals[indices[i + 1]] += normal;
    normals[indices[i + 2]] += normal;
  }

  for (std::size_t i = 0; i < vertices.size(); i++) {
    vertices[i].normal = normals[i] != glm::vec3(0) ? glm::normalize(normals[i]) : glm::vec3(0, 1.0f, 0);
    NNL_ENSURES_DBG(utl::math::IsFinite(vertices[i].normal));
  }
}

void SMesh::GenerateFlatNormals() {
  NNL_EXPECTS(indices.size() % 3 == 0);

  uses_normal = true;

  std::vector<u32> new_indices(indices.size());
  std::vector<SVertex> new_vertices(indices.size());

  for (std::size_t i = 0; i < indices.size(); i += 3) {
    new_indices[i] = i;
    new_indices[i + 1] = i + 1;
    new_indices[i + 2] = i + 2;

    new_vertices[i] = vertices[indices[i]];
    new_vertices[i + 1] = vertices[indices[i + 1]];
    new_vertices[i + 2] = vertices[indices[i + 2]];

    auto& v1 = new_vertices[i];
    auto& v2 = new_vertices[i + 1];
    auto& v3 = new_vertices[i + 2];

    glm::vec3 p1 = v1.position;
    glm::vec3 p2 = v2.position;
    glm::vec3 p3 = v3.position;

    auto a = p3 - p2;
    auto b = p1 - p2;

    glm::vec3 cross = glm::cross(a, b);

    glm::vec3 normal = cross != glm::vec3(0) ? glm::normalize(cross) : glm::vec3(0, 1.0f, 0);

    NNL_ENSURES_DBG(utl::math::IsFinite(normal));

    v1.normal = normal;
    v2.normal = normal;
    v3.normal = normal;
  }

  indices = std::move(new_indices);
  vertices = std::move(new_vertices);
}

glm::mat4 SBone::GetTransform() const { return utl::math::Compose(scale, rotation, translation); }

void SBone::SetTransform(const glm::mat4& transform) {
  std::tie(scale, rotation, translation) = utl::math::Decompose(transform);
}

static void CalculateGlobalMatrices_(const SBone& root, std::vector<glm::mat4>& matrices, glm::mat4 accum) {
  NNL_EXPECTS_DBG(utl::math::IsFinite(accum));
  matrices.push_back(accum * root.GetTransform());

  for (auto& child : root.children) CalculateGlobalMatrices_(child, matrices, accum * root.GetTransform());
}

std::vector<glm::mat4> SSkeleton::GetGlobalMatrices() const {
  std::vector<glm::mat4> skinning_matrices;

  for (auto& root : roots) CalculateGlobalMatrices_(root, skinning_matrices, glm::mat4(1.0f));

  return skinning_matrices;
}

std::vector<glm::mat4> SSkeleton::GetSkinningMatrices() const {
  std::vector<glm::mat4> final_matrices;
  auto bone_refs = GetBoneRefs();

  auto global_matrices = GetGlobalMatrices();

  for (std::size_t i = 0; i < bone_refs.size(); i++)
    final_matrices.push_back(global_matrices.at(i) * bone_refs[i].get().inverse);

  return final_matrices;
}

void SSkeleton::UpdateInverseMatrices() {
  auto bone_refs = GetBoneRefs();

  auto skinning_matrices = GetGlobalMatrices();

  for (std::size_t i = 0; i < bone_refs.size(); i++)
    bone_refs[i].get().inverse = utl::math::Inverse(skinning_matrices.at(i));
}

template <typename TBone>
static void CalculateBoneRefs_(TBone& root, std::vector<std::reference_wrapper<TBone>>& refs) {
  refs.push_back(std::ref(root));

  for (auto& child : root.children) CalculateBoneRefs_(child, refs);
}

std::vector<std::reference_wrapper<const SBone>> SSkeleton::GetBoneRefs() const {
  std::vector<std::reference_wrapper<const SBone>> refs;

  for (auto& root : roots) CalculateBoneRefs_(root, refs);
  return refs;
}

std::vector<std::reference_wrapper<SBone>> SSkeleton::GetBoneRefs() {
  std::vector<std::reference_wrapper<SBone>> refs;

  for (auto& root : roots) CalculateBoneRefs_(root, refs);
  return refs;
}

bool SMaterial::operator==(const SMaterial& rhs) const {
  return specular == rhs.specular && diffuse == rhs.diffuse && ambient == rhs.ambient && emissive == rhs.emissive &&
         specular_power == rhs.specular_power && texture_id == rhs.texture_id && vfx_group_id == rhs.vfx_group_id &&
         alpha_mode == rhs.alpha_mode && lit == rhs.lit && projection_mode == rhs.projection_mode &&
         opacity == rhs.opacity;
}

void SModel::Transform(const glm::mat4& matrix) {
  for (auto& smesh : meshes) smesh.Transform(matrix);
}

void SModel::TransformUV(const glm::mat3& matrix) {
  for (auto& smesh : meshes) smesh.TransformUV(matrix);
}

bool SModel::TryBakeBindShape() {
  auto bind_shape_mats = skeleton.GetSkinningMatrices();

  for (std::size_t i = 0; i < bind_shape_mats.size(); i++) {
    float det = glm::determinant(bind_shape_mats[i]);
    if (det == 0.0f) {
      return false;
    }
  }

  for (auto& smesh : meshes) {
    for (auto& vertex : smesh.vertices) {
      glm::mat4 average_transform = glm::mat4(0.0f);

      for (std::size_t i = 0; i < kMaxNumBoneWeight; i++) {
        auto pre_transform = bind_shape_mats.at(vertex.bones[i]);
        average_transform += pre_transform * vertex.weights[i];
      }

      if (average_transform != glm::mat4(0)) {
        vertex = average_transform * vertex;
      }
    }
  }

  // Remove bind shape matrices from inverse matrices

  auto bone_refs = skeleton.GetBoneRefs();

  for (std::size_t i = 0; i < bone_refs.size(); i++) {
    auto& bone = bone_refs.at(i).get();
    auto& pretransform = bind_shape_mats.at(i);

    glm::mat4 pretransform_inv = utl::math::Inverse(pretransform);

    bone.inverse = bone.inverse * pretransform_inv;
  }

  return true;
}

glm::vec3 SModel::CalculateCenter() const {
  glm::vec3 accum{0};
  for (auto& smesh : meshes) accum += smesh.CalculateCenter();
  return accum / static_cast<float>(meshes.size());
}

std::pair<glm::vec3, glm::vec3> SModel::FindMinMax() const {
  float minX = std::numeric_limits<float>::max();
  float maxX = std::numeric_limits<float>::lowest();

  float minY = std::numeric_limits<float>::max();
  float maxY = std::numeric_limits<float>::lowest();

  float minZ = std::numeric_limits<float>::max();
  float maxZ = std::numeric_limits<float>::lowest();

  for (const auto& mesh : meshes) {
    auto [min, max] = mesh.FindMinMax();

    if (max.x > maxX) maxX = max.x;
    if (min.x < minX) minX = min.x;

    if (max.y > maxY) maxY = max.y;
    if (min.y < minY) minY = min.y;

    if (max.z > maxZ) maxZ = max.z;
    if (min.z < minZ) minZ = min.z;
  }

  return std::make_pair(glm::vec3(minX, minY, minZ), glm::vec3(maxX, maxY, maxZ));
}

glm::mat4 SModel::Normalize() {
  auto [min, max] = FindMinMax();

  float scaler =
      std::max({glm::abs(min.x), glm::abs(max.x), glm::abs(min.y), glm::abs(max.y), glm::abs(min.z), glm::abs(max.z)});

  if (scaler <= 1.0f) return glm::mat4(1.0f);

  glm::mat4 scale_mat = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f / scaler));

  for (auto& smesh : meshes) {
    for (auto& vertex : smesh.vertices) {
      vertex.position = scale_mat * glm::vec4(vertex.position, 1.0f);
    }
  }

  glm::mat4 inv_scale_mat = glm::scale(glm::mat4(1.0f), glm::vec3(scaler));

  auto bone_refs = skeleton.GetBoneRefs();

  for (auto& bone_ref : bone_refs) {
    auto& bone = bone_ref.get();
    bone.inverse = bone.inverse * inv_scale_mat;
  }

  return inv_scale_mat;
}

std::vector<std::pair<glm::vec2, glm::vec2>> SModel::NormalizeUV() {
  const glm::vec2 default_translation{-0.0f};
  const glm::vec2 default_scale{1.0f};

  // Ppairs of scale and translation for each mesh group
  std::vector<std::pair<glm::vec2, glm::vec2>> uv_transforms;

  // Find the minimal uv coordinates
  for (auto& smesh : meshes) {
    auto [min, max] = smesh.FindMinMaxUV();

    if (uv_transforms.size() <= smesh.mesh_group)
      uv_transforms.resize(smesh.mesh_group + 1, std::make_pair(default_scale, default_translation));

    auto& uv_transform = uv_transforms.at(smesh.mesh_group);

    if (min.x < uv_transform.first.x) uv_transform.first.x = min.x;

    if (min.y < uv_transform.first.y) uv_transform.first.y = min.y;
  }

  // Bring them into the positive range
  for (auto& smesh : meshes) {
    auto& uv_transform = uv_transforms.at(smesh.mesh_group);

    glm::mat3 matrix{1.0f};

    // Translate all UV's to make them >= 0.0f
    matrix[2][0] = -1.0f * uv_transform.first.x;
    matrix[2][1] = -1.0f * uv_transform.first.y;

    smesh.TransformUV(matrix);

    auto [min, max] = smesh.FindMinMaxUV();

    assert(min.x >= 0.0f && min.y >= 0.0f);

    if (max.x > uv_transform.second.x) uv_transform.second.x = max.x;

    if (max.y > uv_transform.second.y) uv_transform.second.y = max.y;
  }

  // Scale down UVs into the range [0.0, 1.0f]
  for (auto& smesh : meshes) {
    auto& uv_transform = uv_transforms.at(smesh.mesh_group);

    glm::mat3 matrix{1.0f};

    // Scale by the inverse of the max coord
    matrix[0][0] = uv_transform.second.x > 0.0f ? 1.0f / uv_transform.second.x : 1.0f;
    matrix[1][1] = uv_transform.second.y > 0.0f ? 1.0f / uv_transform.second.y : 1.0f;

    smesh.TransformUV(matrix);
  }

  return uv_transforms;
}

void SModel::NormalizeWeights() {
  for (auto& smesh : meshes) {
    smesh.NormalizeWeights();
  }
}

void SModel::NormalizeNormals() {
  for (auto& smesh : meshes) {
    smesh.NormalizeNormals();
  }
}

void SModel::SortWeights() {
  for (auto& mesh : meshes) {
    mesh.SortWeights();
  }
}

void SModel::QuantWeights(unsigned int steps) {
  NNL_EXPECTS(steps > 0);
  for (auto& mesh : meshes) {
    mesh.QuantWeights(steps);
  }
}

void SModel::LimitWeightsPerVertex(unsigned int max_weights) {
  NNL_EXPECTS(max_weights > 0 && max_weights <= kMaxNumBoneWeight);
  for (auto& mesh : meshes) {
    mesh.LimitWeightsPerVertex(max_weights);
  }
}

void SModel::LimitWeightsPerTriangle(unsigned int max_weights) {
  NNL_EXPECTS(max_weights > 0);
  for (auto& mesh : meshes) {
    mesh.LimitWeightsPerTriangle(max_weights);
  }
}

void SModel::ResetWeights() {
  for (auto& mesh : meshes) {
    mesh.ResetWeights();
  }
}

void SModel::ResetNormals() {
  for (auto& mesh : meshes) {
    mesh.ResetNormals();
  }
}

void SModel::ResetUVs() {
  for (auto& mesh : meshes) {
    mesh.ResetUVs();
  }
}

void SModel::ResetColors() {
  for (auto& mesh : meshes) {
    mesh.ResetColors();
  }
}

void SModel::RemoveDuplicateMaterials() {
  std::vector<SMaterial> new_materials;

  new_materials.reserve(materials.size());

  auto find_index = [&new_materials, this](std::size_t material_index) -> std::size_t {
    auto& material = materials.at(material_index);
    auto itr_material = std::find(new_materials.begin(), new_materials.end(), material);
    auto new_material_index = static_cast<std::size_t>(std::distance(new_materials.begin(), itr_material));

    if (itr_material == new_materials.end()) new_materials.push_back(material);

    return new_material_index;
  };

  for (auto& mesh : meshes) {
    std::map<std::size_t, std::vector<std::size_t>> new_variants;

    mesh.material_id = find_index(mesh.material_id);

    for (auto& [new_mat_id, variants] : mesh.material_variants) {
      new_variants.insert({find_index(new_mat_id), variants});
    }

    assert(new_variants.size() <= mesh.material_variants.size());

    mesh.material_variants = new_variants;
  }

  std::vector<SUVAnimation> new_uv_anims;
  new_uv_anims.reserve(uv_animations.size());

  for (SUVAnimation& suvanim : uv_animations) {
    SUVAnimation new_suv_anim;
    new_suv_anim.name = suvanim.name;
    for (auto& [mat_target, channel] : suvanim.translation_channels) {
      auto new_mat_index = find_index(mat_target);
      new_suv_anim.translation_channels.insert({new_mat_index, std::move(channel)});
    }

    new_uv_anims.push_back(std::move(new_suv_anim));
  }

  assert(new_uv_anims.size() == uv_animations.size());

  NNL_ENSURES_DBG(new_materials.size() <= materials.size());

  uv_animations = std::move(new_uv_anims);
  materials = std::move(new_materials);
}

void SModel::GenerateSmoothNormals() {
  for (auto& smesh : meshes) smesh.GenerateSmoothNormals();
}

void SModel::GenerateFlatNormals() {
  for (auto& smesh : meshes) smesh.GenerateFlatNormals();
}

}  // namespace nnl
