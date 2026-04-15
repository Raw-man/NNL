#include "NNL/simple_asset/smodel.hpp"

#include "NNL/utility/math.hpp"
namespace nnl {

glm::mat4 SAttachment::GetTransform() const { return utl::math::Compose(scale, rotation, translation); }

void SAttachment::SetTransform(const glm::mat4& transform) {
  std::tie(scale, rotation, translation) = utl::math::Decompose(transform);
}

void SVertex::Transform(const glm::mat4& transform) {
  NNL_EXPECTS_DBG(utl::math::IsFinite(transform));
  NNL_EXPECTS_DBG(utl::math::IsFinite(position));
  NNL_EXPECTS_DBG(utl::math::IsFinite(normal));

  position = transform * glm::vec4(position, 1);

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
  NNL_EXPECTS_DBG(steps > 0 && steps <= 1'000'000);
  NNL_EXPECTS_DBG(!weights.empty());

  float sum = 0;

  for (auto& weight : weights) {
    weight.second = std::round(weight.second * steps);
    sum += weight.second;
  }

  if (sum != steps) {
    auto max_weight_itr = std::max_element(weights.begin(), weights.end(),
                                           [](const auto& lhs, const auto& rhs) { return lhs.second < rhs.second; });

    auto& max_weight = (*max_weight_itr).second;

    float remainder = steps - sum;

    max_weight += remainder;
  }

  for (auto& weight : weights) {
    weight.second = weight.second / steps;
  }
}

void SVertex::LimitWeights(unsigned int max_weights) {
  NNL_EXPECTS_DBG(max_weights > 0);
  while (weights.size() > max_weights) {
    auto min_weight_itr = std::min_element(weights.cbegin(), weights.cend(),
                                           [](const auto& lhs, const auto& rhs) { return lhs.second < rhs.second; });

    auto min_weight_index = min_weight_itr - weights.begin();

    auto min_weight = (*min_weight_itr).second;

    weights.erase(weights.begin() + min_weight_index);

    float weight_remainder = min_weight / weights.size();

    // Make sure weights add up to 1
    for (auto& weight : weights) {
      weight.second = (double)weight.second + weight_remainder;
    }
  }
}

void SVertex::RemoveZeroWeights() {
  decltype(weights) new_weights;

  for (auto& weight : weights) {
    if (weight.second > 0.0f) new_weights.push_back(weight);
  }

  weights = std::move(new_weights);
}

void SVertex::ResetWeights() { weights.clear(); }

void SVertex::ResetNormals() { normal = {0.0f, 1.0f, 0.0f}; }

void SVertex::ResetUVs() { uv = {0.0f, 0.0f}; }

void SVertex::ResetColors() { color = {0xFF, 0xFF, 0xFF, 0xFF}; }

bool SVertex::HasAlpha() const { return color.a < NNL_ALPHA_OPAQ_F; }

void SVertex::NormalizeWeights() {
  double sum = 0.0f;

  for (auto& weight : weights) sum += weight.second;

  for (auto& weight : weights) weight.second /= sum;
}

utl::static_vector<u16, 8> SVertex::GetBoneIndices() const {
  utl::static_vector<u16, 8> bones(weights.size());

  for (std::size_t i = 0; i < weights.size(); i++) {
    bones[i] = weights[i].first;
  }

  std::sort(bones.begin(), bones.begin() + weights.size());

  return bones;
}

bool SVertex::IsApproxEqual(const SVertex& rhs) const {
  if (!utl::math::IsApproxEqual(position, rhs.position)) return false;

  if (!utl::math::IsApproxEqual(uv, rhs.uv)) return false;

  if (!utl::math::IsApproxEqual(normal, rhs.normal)) return false;

  if (color != rhs.color) return false;

  if (weights.size() != rhs.weights.size()) return false;

  if (GetBoneIndices() != rhs.GetBoneIndices()) return false;

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
      const auto& vertex = triangle[i];

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

std::set<u16> SMesh::GetBoneIndices() const {
  std::set<u16> unique_ids;
  for (auto& vertex : vertices) {
    for (const auto& weight : vertex.weights) {
      unique_ids.insert(weight.first);
    }
  }
  return unique_ids;
}

std::vector<u32> SMesh::GetVerticesByBone(u16 bone_id) const {
  std::set<u32> unique_ids;
  for (std::size_t i = 0; i < vertices.size(); i++) {
    auto& vertex = vertices[i];
    for (auto& weight : vertex.weights) {
      if (weight.first == bone_id) {
        unique_ids.insert(i);
      }
    }
  }

  return std::vector<u32>(unique_ids.begin(), unique_ids.end());
}

bool SMesh::IsAffectedByFewBones() const {
  std::set<u16> unique_ids;
  for (auto& vertex : vertices) {
    for (const auto& weight : vertex.weights) {
      unique_ids.insert(weight.first);
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

void SMesh::RemoveZeroWeights() {
  for (auto& vertex : vertices) {
    vertex.RemoveZeroWeights();
  }
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
  NNL_EXPECTS(steps > 0 && steps <= 1'000'000);
  for (auto& vertex : vertices) {
    vertex.QuantWeights(steps);
  }
}

void SMesh::LimitWeightsPerVertex(unsigned int max_weights) {
  NNL_EXPECTS(max_weights > 0);
  for (auto& vertex : vertices) {
    vertex.LimitWeights(max_weights);
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

void SMesh::LimitWeightsPerTriangle(unsigned int max_weights) {
  NNL_EXPECTS(max_weights > 0);
  NNL_EXPECTS(indices.size() % 3 == 0);
  for (std::size_t i = 0; i < indices.size(); i += 3) {
    std::vector<std::reference_wrapper<SVertex>> triangle_vertices;

    std::set<u16> unique_bones;
    // Get unique bones of the primitive
    for (std::size_t j = i; j < i + 3; j++) {
      auto& vertex = vertices.at(indices[j]);

      // If there's only 1 weight, it shouldn't be removed
      if (vertex.weights.size() > 1) triangle_vertices.push_back(vertex);

      for (auto& weight : vertex.weights) unique_bones.insert(weight.first);
    }

    // Go to the next iteration if the size of unique bones doesn't exceed the limit
    // or no weights to remove
    if (unique_bones.size() <= max_weights || triangle_vertices.size() == 0) continue;

    std::unordered_map<u16, float> bone_infos;  // bone id, total_weight

    // Calculate the sum of all weights of a bone in a triangle
    for (auto& vertex_ref : triangle_vertices) {
      auto& vertex = vertex_ref.get();

      for (auto& weight : vertex.weights) {
        bone_infos[weight.first] += weight.second;
      }
    }

    // Find least influential bone id (with the smallest weight)
    u16 least_influential_bone_id = 0;

    float min_total_weight = std::numeric_limits<double>::max();

    for (auto [bone_id, total_weight] : bone_infos) {
      if (total_weight < min_total_weight) {
        min_total_weight = total_weight;
        least_influential_bone_id = bone_id;
      }
    }

    // Delete this bone from weight data
    for (auto& vertex_ref : triangle_vertices) {
      auto& vertex = vertex_ref.get();

      auto min_weight_itr =
          std::find_if(vertex.weights.begin(), vertex.weights.end(),
                       [least_influential_bone_id](auto& lhs) { return lhs.first == least_influential_bone_id; });

      // If the bone was not found in this vertex, go to the next one
      if (min_weight_itr == vertex.weights.end()) continue;

      auto min_weight_index = min_weight_itr - vertex.weights.begin();

      if (vertex.weights.size() > 1) {
        double min_weight = std::get<1>(*min_weight_itr);

        vertex.weights.erase(vertex.weights.begin() + min_weight_index);

        double weight_remainder = min_weight / (double)vertex.weights.size();
        // Normalize weights

        for (auto& weight : vertex.weights) {
          weight.second += (float)weight_remainder;
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
    const auto& vertex = vertices.at(indices[i]);

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
      for (auto& weight : vertex.weights) {
        auto pre_transform = bind_shape_mats.at(weight.first);
        average_transform += pre_transform * weight.second;
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

void SModel::QuantWeights(unsigned int steps) {
  NNL_EXPECTS(steps > 0 && steps <= 1'000'000);
  for (auto& mesh : meshes) {
    mesh.QuantWeights(steps);
  }
}

void SModel::LimitWeightsPerVertex(unsigned int max_weights) {
  NNL_EXPECTS(max_weights > 0);
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

void SModel::RemoveZeroWeights() {
  for (auto& mesh : meshes) {
    mesh.RemoveZeroWeights();
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
