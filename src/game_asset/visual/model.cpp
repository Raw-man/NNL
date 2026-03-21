#define _USE_MATH_DEFINES

#include "NNL/game_asset/visual/model.hpp"

#include <tri_stripper.h>

#include <cmath>
#include <glm/gtc/type_ptr.hpp>
#include <numeric>

#include "NNL/common/io.hpp"
#include "NNL/game_asset/visual/vertexde.hpp"
#include "NNL/utility/color.hpp"
#include "NNL/utility/math.hpp"
#include "NNL/utility/string.hpp"
namespace nnl {
namespace model {

struct Hash {
  std::size_t operator()(const UVAnimation& key) const noexcept {
    return static_cast<std::size_t>(key.animation_mode) | static_cast<std::size_t>(key.num_frames);
  }
};

bool operator==(const UVAnimation& rhs, const UVAnimation& lhs) {
  return rhs.animation_mode == lhs.animation_mode && rhs.num_frames == lhs.num_frames &&
         utl::math::IsApproxEqual(rhs.v_shift, lhs.v_shift) && utl::math::IsApproxEqual(rhs.u_shift, lhs.u_shift);
}

bool operator<(const Material& lhs, const Material& rhs) {
  if (lhs.diffuse != rhs.diffuse) return lhs.diffuse < rhs.diffuse;
  if (lhs.ambient != rhs.ambient) return lhs.ambient < rhs.ambient;
  if (lhs.specular != rhs.specular) return lhs.specular < rhs.specular;
  if (lhs.emissive != rhs.emissive) return lhs.emissive < rhs.emissive;
  if (lhs.specular_power != rhs.specular_power) return lhs.specular_power < rhs.specular_power;
  if (lhs.texture_id != rhs.texture_id) return lhs.texture_id < rhs.texture_id;
  if (lhs.uv_animation_id != rhs.uv_animation_id) return lhs.uv_animation_id < rhs.uv_animation_id;
  if (lhs.vfx_group_id != rhs.vfx_group_id) return lhs.vfx_group_id < rhs.vfx_group_id;
  return lhs.features < rhs.features;
}

std::vector<STriangle> ExtractTriangles(const SubMesh& submesh, bool skip_degenerate) {
  std::vector<STriangle> triangles;

  bool indexed = vertexde::IsIndexed(submesh.vertex_format);

  if (!indexed) {
    for (const auto& primitive : submesh.primitives) {
      if (primitive.num_elements < 3) continue;

      std::vector<SVertex> vertices =
          vertexde::Decode(primitive.vertex_index_buffer, submesh.vertex_format, submesh.bone_indices);

      if (primitive.primitive_type == PrimitiveType::kTriangleStrip) {
        for (u16 v = 0; v < primitive.num_elements - 2; v++) {
          if (v % 2 == 0) {
            auto triangle = STriangle{{vertices[v + 0], vertices[v + 1], vertices[v + 2]}};
            if (!triangle.IsDegenerate() || !skip_degenerate) triangles.push_back(std::move(triangle));
          } else {
            auto triangle = STriangle{{vertices[v + 1], vertices[v + 0], vertices[v + 2]}};
            if (!triangle.IsDegenerate() || !skip_degenerate) triangles.push_back(std::move(triangle));
          }
        }
      }

      else if (primitive.primitive_type == PrimitiveType::kTriangles) {
        for (u32 v = 0; v < primitive.num_elements; v += 3) {
          auto triangle = STriangle{{vertices[v + 0], vertices[v + 1], vertices[v + 2]}};
          if (!triangle.IsDegenerate() || !skip_degenerate) triangles.push_back(std::move(triangle));
        }
      }
    }
  } else {
    std::vector<SVertex> vertex_reader =
        vertexde::Decode(submesh.indexed_vertex_buffer, submesh.vertex_format, submesh.bone_indices);

    for (const auto& primitive : submesh.primitives) {
      if (primitive.num_elements < 3) continue;

      std::vector<u16> indices;

      if (vertexde::Is16Indices(submesh.vertex_format))
        indices = utl::data::ReinterpretContainer<u16>(primitive.vertex_index_buffer);
      else
        indices = utl::data::CastContainer<u16>(primitive.vertex_index_buffer);

      if (primitive.primitive_type == PrimitiveType::kTriangleStrip) {
        for (u16 v = 0; v < primitive.num_elements - 2; v++) {
          auto a = indices.at(v + 0);
          auto b = indices.at(v + 1);
          auto c = indices.at(v + 2);

          if (v % 2 == 0) {
            auto triangle = STriangle{{vertex_reader[a], vertex_reader[b], vertex_reader[c]}};
            if (!((a == b) || (b == c) || (a == c)) || !skip_degenerate) triangles.push_back(std::move(triangle));
          } else {
            auto triangle = STriangle{{vertex_reader[b], vertex_reader[a], vertex_reader[c]}};
            if (!((a == b) || (b == c) || (a == c)) || !skip_degenerate) triangles.push_back(std::move(triangle));
          }
        }
      }

      else if (primitive.primitive_type == PrimitiveType::kTriangles) {
        for (u16 v = 0; v < primitive.num_elements; v += 3) {
          auto a = indices.at(v + 0);
          auto b = indices.at(v + 1);
          auto c = indices.at(v + 2);

          auto triangle = STriangle{{vertex_reader[a], vertex_reader[b], vertex_reader[c]}};

          if (!((a == b) || (b == c) || (a == c)) || !skip_degenerate) triangles.push_back(std::move(triangle));
        }
      }
    }
  }

  return triangles;
}

auto GetBones(u32 start_index, const std::vector<u32>& indices, const std::vector<SVertex>& vertices) {
  NNL_EXPECTS_DBG(start_index + 3 <= indices.size());
  decltype(TriangleGroup::bones) bones;

  for (std::size_t i = 0; i < 3; i++) {
    auto& vertex = vertices.at(indices[start_index + i]);

    NNL_EXPECTS_DBG(!vertex.weights.empty());

    for (auto& weight : vertex.weights) {
      bones.Insert(weight.first);
    }
  }

  NNL_ENSURES_DBG(!bones.IsEmpty());

  return bones;
}

struct Comp {
  bool operator()(const TriangleGroup& lhs, const TriangleGroup& rhs) const {
    if (lhs.bones.Size() != rhs.bones.Size()) return lhs.bones.Size() < rhs.bones.Size();

    for (std::size_t i = 0; i < lhs.bones.Size(); i++) {
      if (lhs.bones[i] != rhs.bones[i]) {
        return lhs.bones[i] < rhs.bones[i];
      }
    }

    return false;
  }
};

std::vector<TriangleGroup> GroupTrianglesByBones(const std::vector<u32>& indices, const std::vector<SVertex>& vertices,
                                                 u32 max_num_triangles, bool join_bone_sets) {
  NNL_EXPECTS(!vertices.empty());
  NNL_EXPECTS(!indices.empty());
  NNL_EXPECTS(indices.size() % 3 == 0);

  // Create initial bone sets
  std::set<TriangleGroup, Comp> init_groups;
  std::vector<decltype(TriangleGroup::bones)> triangle_bones(indices.size() / 3);

  for (std::size_t i = 0; i < indices.size(); i += 3) {
    TriangleGroup group;
    group.bones = GetBones(i, indices, vertices);
    triangle_bones[i / 3] = group.bones;
    init_groups.insert(group);
    assert(!group.bones.IsEmpty());
  }
  std::vector<TriangleGroup> triangle_groups(init_groups.begin(), init_groups.end());

  // Remove sets that are subsets of the other sets
  {
    std::vector<TriangleGroup> unique_triangle_groups;

    unique_triangle_groups.reserve(triangle_groups.size());

    for (std::size_t i = 0; i < triangle_groups.size(); i++) {
      auto& triangle_group = triangle_groups[i];

      bool unique_group = true;

      for (std::size_t j = i + 1; j < triangle_groups.size(); j++) {
        auto& triangle_group_j = triangle_groups[j];
        // At this point, 2 sets only have the same size if they are different.
        if (triangle_group_j.bones.Size() != triangle_group.bones.Size() &&
            triangle_group_j.bones.IsSubset(triangle_group.bones)) {
          unique_group = false;
          break;
        }
      }

      if (unique_group) {
        unique_triangle_groups.push_back(triangle_group);
      }
    }

    triangle_groups = std::move(unique_triangle_groups);
  }

  // Try to further join different sets
  if (join_bone_sets) {
    std::vector<TriangleGroup> triangle_groups_not_joinable;

    triangle_groups_not_joinable.reserve(triangle_groups.size());

    while (triangle_groups.size() > 0)

    {
      unsigned long best_index = -1;
      unsigned long best_intersection_size = 0;
      float best_common_ratio = 0.0f;
      auto& triangle_group = triangle_groups[0];

      decltype(TriangleGroup::bones) bone_set_to_join;

      for (std::size_t j = 1; j < triangle_groups.size(); j++) {
        auto& triangle_group_j = triangle_groups[j];

        auto intersect = triangle_group.bones.Intersect(triangle_group_j.bones);

        std::size_t joined_size = (triangle_group.bones.Size() + triangle_group_j.bones.Size() - intersect.Size());
        float common_ratio = (float)intersect.Size() / (float)joined_size;
        // Join groups that have at least a few shared bone indices
        if (common_ratio > 0.30f && common_ratio > best_common_ratio && joined_size <= kMaxNumBonePerPrim) {
          best_index = j;
          best_intersection_size = intersect.Size();
          bone_set_to_join = triangle_group_j.bones;
          best_common_ratio = common_ratio;
        }
      }

      if (best_intersection_size == 0) {
        assert(!triangle_groups.empty());
        triangle_groups_not_joinable.push_back(triangle_group);
        triangle_groups.erase(triangle_groups.begin() + 0);

      } else {
        assert(triangle_groups.size() >= 2);
        assert(best_index < triangle_groups.size());
        TriangleGroup new_triangle_group;
        new_triangle_group.bones = triangle_group.bones.Join(bone_set_to_join);
        triangle_groups.erase(triangle_groups.begin() + best_index);
        triangle_groups.erase(triangle_groups.begin() + 0);

        triangle_groups.insert(triangle_groups.end(), new_triangle_group);
      }
    }
    assert(triangle_groups.empty());
    triangle_groups = std::move(triangle_groups_not_joinable);
  }

  // Put triangles into the groups
  for (std::size_t i = 0; i < indices.size(); i += 3) {
    std::size_t j = 0;
    for (; j < triangle_groups.size(); j++) {
      auto& triangle_group = triangle_groups[j];
      if (triangle_group.bones.IsSubset(triangle_bones[i / 3])) {
        triangle_group.indices.insert(triangle_group.indices.end(), indices.begin() + i, indices.begin() + i + 3);
        break;
      }
    }
    assert(j < triangle_groups.size());
  }

  // Remove unused unused/empty groups
  {
    std::vector<TriangleGroup> triangle_groups_used;
    triangle_groups_used.reserve(triangle_groups.size());

    for (auto& triangle_group : triangle_groups) {
      if (triangle_group.indices.size() > 0) triangle_groups_used.push_back(std::move(triangle_group));
    }

    triangle_groups = std::move(triangle_groups_used);
  }

  // If the num of triangles in a group exceeds the limit, slice further

  for (std::size_t i = 0; i < triangle_groups.size(); i++) {
    if (triangle_groups[i].indices.size() > max_num_triangles * 3) {
      TriangleGroup& triangle_group = triangle_groups.emplace_back();

      auto itr_begin = triangle_groups[i].indices.begin();

      auto itr_new_tri_beg = itr_begin + max_num_triangles * 3;

      auto itr_new_tri_end = itr_begin + triangle_groups[i].indices.size();

      triangle_group.indices = std::vector<u32>(itr_new_tri_beg, itr_new_tri_end);

      triangle_groups[i].indices.resize(max_num_triangles * 3);
      // Make sure new groups use only bones they need
      decltype(TriangleGroup::bones) bones;

      for (std::size_t j = 0; j < triangle_groups[i].indices.size(); j += 3) {
        bones = bones.Join(GetBones(j, triangle_groups[i].indices, vertices));
      }

      triangle_groups[i].bones = bones;

      bones.Clear();

      for (std::size_t j = 0; j < triangle_group.indices.size(); j += 3) {
        bones = bones.Join(GetBones(j, triangle_group.indices, vertices));
      }

      triangle_group.bones = bones;
    }
  }

  std::sort(triangle_groups.begin(), triangle_groups.end(),
            [](const auto& a, const auto& b) -> bool { return a.bones.Size() < b.bones.Size(); });

  return triangle_groups;
}

BBox GenerateBBox(const SMesh& mesh, glm::mat4 transform) {
  BBox bbox;

  auto [min, max] = mesh.FindMinMax();

  bbox.min = transform * glm::vec4(min, 1.0f);
  bbox.max = transform * glm::vec4(max, 1.0f);

  return bbox;
}

Mesh Convert(SMesh&& smesh, const ConvertParam& param) {
  NNL_EXPECTS(smesh.indices.size() % 3 == 0);
  NNL_EXPECTS(!smesh.vertices.empty());
  NNL_EXPECTS(!smesh.indices.empty());

  smesh.LimitWeightsPerVertex(4);
  smesh.LimitWeightsPerTriangle();
  smesh.NormalizeWeights();

  if ((param.compress_lvl != CompLvl::kNone && param.force_vertex_format == 0) ||
      vertexde::Is8Weights(param.force_vertex_format))
    smesh.QuantWeights(128);

  if (vertexde::Is16Weights(param.force_vertex_format)) smesh.QuantWeights(32768);

  Mesh mesh;

  u32 vertex_format = param.force_vertex_format;

  if (vertex_format == 0) {
    vertex_format =
        (param.compress_lvl == CompLvl::kNone
             ? vertexde::fmt::kPosition32
             : (param.compress_lvl == CompLvl::kMedium ? vertexde::fmt::kPosition16 : vertexde::fmt::kPosition8));

    if (param.indexed) {
      if (param.compress_lvl == CompLvl::kMax && smesh.vertices.size() <= 256)
        vertex_format |= vertexde::fmt::kIndex8;
      else
        vertex_format |= vertexde::fmt::kIndex16;
    }

    if (smesh.IsAffectedByFewBones())
      vertex_format |= (param.compress_lvl == CompLvl::kNone ? vertexde::fmt::kWeight32 : vertexde::fmt::kWeight8);

    if (smesh.uses_uv)
      vertex_format |= (param.compress_lvl == CompLvl::kNone
                            ? vertexde::fmt::kUV32
                            : (param.compress_lvl == CompLvl::kMedium ? vertexde::fmt::kUV16 : vertexde::fmt::kUV8));
    ;

    if (smesh.uses_normal)
      vertex_format |= (param.compress_lvl == CompLvl::kNone ? vertexde::fmt::kNormal32 : vertexde::fmt::kNormal8);

    if (smesh.uses_color)
      vertex_format |= (param.compress_lvl == CompLvl::kNone ? vertexde::fmt::kColor8888 : vertexde::fmt::kColor4444);
  }
  NNL_EXPECTS(vertexde::HasPosition(vertex_format));
  NNL_EXPECTS(vertexde::GetMorphNum(vertex_format) == 1);  // > 1 not supported

  // The draw command can draw up to 65536 vertices. However, if u8 indices are
  // used the max number of vertices in a vertex buffer cannot exceed 256.
  u32 vertex_limit = vertexde::IsIndexed(vertex_format)
                         ? glm::pow(2, vertexde::GetIndexFormat(vertex_format) * CHAR_BIT)
                         : std::numeric_limits<u16>::max() + 1;

  auto triangle_groups = GroupTrianglesByBones(smesh.indices, smesh.vertices, vertex_limit / 3, param.join_submeshes);

  for (auto& [bones, triangles] : triangle_groups) {
    u32 vertex_type_local = vertex_format;

    auto [indices, vertices] = SMesh::RemoveDuplicateVertices(triangles, smesh.vertices);

    assert(vertices.size() <= vertex_limit);

    SubMesh submesh;

    if (!vertexde::HasWeights(vertex_format)) {
      submesh.num_bones = 1;
    } else {
      submesh.num_bones = bones.Size();
      // If a submesh refers to two or more weights this flag must be set
      mesh.use_skinning = true;
      // Optimization edge case for NSUNI: no need to include weights into vertex data
      // if only 1 bone is used
      if (param.optimize_weights && bones.Size() == 1) {
        vertex_type_local &= (~vertexde::fmt_mask::kWeight);
        vertex_type_local &= (~vertexde::fmt_mask::kWeightNum);
        submesh.num_bones = 0;
      }
    }

    {
      std::size_t i = 0;
      for (auto bone_id : bones) {
        submesh.bone_indices.at(i++) = bone_id;
      }
    }

    // Recalculate weight number

    if (vertexde::HasWeights(vertex_type_local)) {
      // reset weight num to 0;
      vertex_type_local &= (~vertexde::fmt_mask::kWeightNum);
      // set new weight
      vertex_type_local |= vertexde::fmt::kWeightNum(bones.Size());
    }

    submesh.vertex_format = vertex_type_local;

    if (vertexde::IsIndexed(vertex_format)) {
      submesh.indexed_vertex_buffer = vertexde::Encode(vertices, vertex_type_local, submesh.bone_indices);
    }

    if (!param.use_strips) {
      Primitive primitive;

      primitive.primitive_type = PrimitiveType::kTriangles;
      primitive.num_elements = indices.size();  // Num indices

      if (vertexde::IsIndexed(vertex_format)) {
        if (vertexde::Is8Indices(vertex_format))
          primitive.vertex_index_buffer = utl::data::CastContainer<u8>(indices);
        else
          primitive.vertex_index_buffer = utl::data::ReinterpretContainer<u8>(utl::data::CastContainer<u16>(indices));

      } else {
        std::vector<SVertex> plain_vertices;
        plain_vertices.reserve(indices.size());

        for (std::size_t i = 0; i < indices.size(); i++) {
          plain_vertices.push_back(vertices.at(indices[i]));
        }

        primitive.vertex_index_buffer = vertexde::Encode(plain_vertices, vertex_type_local, submesh.bone_indices);
      }

      submesh.display_list.push_back(GeCmd::kSetVertexType | submesh.vertex_format);
      submesh.display_list.push_back(GeCmd::kSetBaseAddress);
      submesh.display_list.push_back(GeCmd::kSetVramAddress);
      submesh.display_list.push_back(GeCmd::kDrawPrimitives | (utl::data::as_int(primitive.primitive_type) << 16) |
                                     primitive.num_elements);
      submesh.primitives.push_back(std::move(primitive));
      mesh.submeshes.push_back(std::move(submesh));

    } else {
      auto indices_long = utl::data::CastContainer<std::size_t>(indices);

      indices.resize(0);
      indices.shrink_to_fit();

      triangle_stripper::tri_stripper stripper(indices_long);

      stripper.SetCacheSize(0);
      stripper.SetMinStripSize(2);
      // stripper.SetBackwardSearch(true); //triggers assert

      triangle_stripper::primitive_vector out_primitives;

      stripper.Strip(&out_primitives);

      while (param.stitch_strips && out_primitives.size() > 1) {
        auto& first_prim = out_primitives[0];
        auto& prim = out_primitives[1];

        if (prim.Type == triangle_stripper::TRIANGLE_STRIP) {
          assert(!first_prim.Indices.empty());

          if (first_prim.Indices.size() % 2 != 0) first_prim.Indices.push_back(first_prim.Indices.back());

          first_prim.Indices.push_back(first_prim.Indices.back());
          first_prim.Indices.push_back(prim.Indices.front());

          first_prim.Indices.insert(first_prim.Indices.end(), prim.Indices.begin(), prim.Indices.end());

          out_primitives.erase(out_primitives.begin() + 1);

        } else
          break;
      }

      for (auto& out_prim : out_primitives) {
        Primitive primitive;

        primitive.num_elements = out_prim.Indices.size();
        primitive.primitive_type = out_prim.Type == triangle_stripper::TRIANGLE_STRIP ? PrimitiveType::kTriangleStrip
                                                                                      : PrimitiveType::kTriangles;

        if (vertexde::IsIndexed(vertex_format)) {
          if (vertexde::Is8Indices(vertex_format))
            primitive.vertex_index_buffer = utl::data::CastContainer<u8>(out_prim.Indices);
          else
            primitive.vertex_index_buffer =
                utl::data::ReinterpretContainer<u8>(utl::data::CastContainer<u16>(out_prim.Indices));
        } else {
          std::vector<SVertex> vertices;
          vertices.reserve(out_prim.Indices.size());

          for (std::size_t i = 0; i < out_prim.Indices.size(); i++) {
            vertices.push_back(vertices.at(out_prim.Indices.at(i)));
          }

          primitive.vertex_index_buffer = vertexde::Encode(vertices, vertex_type_local, submesh.bone_indices);
        }

        submesh.display_list.push_back(GeCmd::kSetVertexType | submesh.vertex_format);
        submesh.display_list.push_back(GeCmd::kSetBaseAddress);
        submesh.display_list.push_back(GeCmd::kSetVramAddress);
        submesh.display_list.push_back(GeCmd::kDrawPrimitives | (utl::data::as_int(primitive.primitive_type) << 16) |
                                       primitive.num_elements);
        submesh.primitives.push_back(std::move(primitive));
      }

      mesh.submeshes.push_back(std::move(submesh));
    }
  }

  if (param.use_bbox) {
    auto used_bones_ids = smesh.GetBoneIndices();

    assert(!used_bones_ids.empty());

    i16 parent_bone_id = *used_bones_ids.begin();

    mesh.use_bbox = param.use_bbox;

    mesh.bbox = GenerateBBox(smesh);

    mesh.bbox.bone_index = parent_bone_id;
  }

  mesh.material_id = smesh.material_id;

  smesh = {};

  return mesh;
}

SMesh Convert(const Mesh& mesh) {
  std::vector<STriangle> triangles;

  bool uses_color = false;
  bool uses_uv = false;
  bool uses_normal = false;

  for (const auto& submesh : mesh.submeshes) {
    assert(vertexde::GetMorphNum(submesh.vertex_format) == 1);
    uses_color |= vertexde::HasColor(submesh.vertex_format);
    uses_uv |= vertexde::HasUV(submesh.vertex_format);
    uses_normal |= vertexde::HasNormal(submesh.vertex_format);

    // Placeholder meshes should preserve at least 1 triangle
    bool skip_deg_triangles =
        !(submesh.primitives.size() != 0 && submesh.primitives[0].num_elements <= 6 && triangles.empty());

    auto submesh_triangles = ExtractTriangles(submesh, skip_deg_triangles);

    triangles.insert(triangles.end(), submesh_triangles.begin(), submesh_triangles.end());
  }

  SMesh smesh = SMesh::FromTriangles(triangles);

  smesh.material_id = mesh.material_id;

  smesh.name += "_mat_" + utl::string::PrependZero(mesh.material_id, 3) + "_bbox_" + std::to_string(mesh.use_bbox);

  smesh.uses_color = uses_color;

  smesh.uses_uv = uses_uv;

  smesh.uses_normal = uses_normal;

  return smesh;
}

ConvertParam GenerateConvertParam(const Mesh& mesh) {
  ConvertParam param;
  param.use_strips = false;
  param.stitch_strips = true;
  for (auto& submesh : mesh.submeshes) {
    if (!submesh.primitives.empty() && submesh.primitives[0].primitive_type == PrimitiveType::kTriangleStrip) {
      param.use_strips = true;
    }

    if (submesh.primitives.size() > 1 && submesh.primitives[1].primitive_type == PrimitiveType::kTriangleStrip) {
      param.use_strips = true;
      param.stitch_strips = false;
    }

    if ((param.force_vertex_format & vertexde::fmt_mask::kIndex) <
        (submesh.vertex_format & vertexde::fmt_mask::kIndex)) {
      param.indexed = true;
      param.force_vertex_format &= ~vertexde::fmt_mask::kIndex;
      param.force_vertex_format |= (submesh.vertex_format & vertexde::fmt_mask::kIndex);
    }

    if ((param.force_vertex_format & vertexde::fmt_mask::kColor) <
        (submesh.vertex_format & vertexde::fmt_mask::kColor)) {
      param.force_vertex_format &= ~vertexde::fmt_mask::kColor;
      param.force_vertex_format |= (submesh.vertex_format & vertexde::fmt_mask::kColor);
    }

    if ((param.force_vertex_format & vertexde::fmt_mask::kNormal) <
        (submesh.vertex_format & vertexde::fmt_mask::kNormal)) {
      param.force_vertex_format &= ~vertexde::fmt_mask::kNormal;
      param.force_vertex_format |= (submesh.vertex_format & vertexde::fmt_mask::kNormal);
    }

    if ((param.force_vertex_format & vertexde::fmt_mask::kPosition) <
        (submesh.vertex_format & vertexde::fmt_mask::kPosition)) {
      param.force_vertex_format &= ~vertexde::fmt_mask::kPosition;
      param.force_vertex_format |= (submesh.vertex_format & vertexde::fmt_mask::kPosition);
    }

    if ((param.force_vertex_format & vertexde::fmt_mask::kUV) < (submesh.vertex_format & vertexde::fmt_mask::kUV)) {
      param.force_vertex_format &= ~vertexde::fmt_mask::kUV;
      param.force_vertex_format |= (submesh.vertex_format & vertexde::fmt_mask::kUV);
    }

    if (submesh.num_bones == 0) {
      param.optimize_weights = true;
    }

    if ((param.force_vertex_format & vertexde::fmt_mask::kWeight) <
        (submesh.vertex_format & vertexde::fmt_mask::kWeight)) {
      param.force_vertex_format &= ~vertexde::fmt_mask::kWeight;
      param.force_vertex_format |= (submesh.vertex_format & vertexde::fmt_mask::kWeight);
    }
  }

  param.use_bbox = mesh.use_bbox;

  return param;
}

std::vector<ConvertParam> GenerateConvertParam(const Model& model) {
  std::vector<ConvertParam> mesh_params;

  for (std::size_t i = 0; i < model.mesh_groups.size(); i++) {
    auto& mesh_group = model.mesh_groups[i];

    for (auto& mesh : mesh_group.meshes) {
      mesh_params.push_back(GenerateConvertParam(mesh));
    }
  }
  return mesh_params;
}

std::vector<Bone> Convert(const SSkeleton& skeleton) {
  std::vector<Bone> bones;
  std::function<void(const SBone&, i16)> Convert;

  Convert = [&Convert, &bones](const SBone& root, i16 parent_index) {
    i16 current_bone_index = utl::data::narrow_cast<i16>(bones.size());

    Bone& rbone = bones.emplace_back();

    rbone.parent_index = parent_index;

    rbone.scale = root.scale;

    rbone.rotation = utl::math::QuatToEuler(root.rotation);

    rbone.translation = root.translation;

    for (auto& child : root.children) {
      Convert(child, current_bone_index);
    }
  };

  for (auto& root : skeleton.roots) Convert(root, -1);

  return bones;
}

void SetBoneNames(SSkeleton& skeleton, const std::vector<std::map<BoneTarget, BoneIndex>>& bone_target_tables,
                  bool is_character) {
  auto bone_refs = skeleton.GetBoneRefs();

  std::map<BoneIndex, std::map<u16, std::vector<std::string>>> bone_names;

  for (std::size_t i = 0; i < bone_target_tables.size(); i++) {
    auto& bone_role_table = bone_target_tables.at(i);

    for (auto [bone_role_id, bone_id] : bone_role_table) {
      if (is_character)
        bone_names[bone_id][i].push_back(bone_target_names.at(bone_role_id).data());
      else
        bone_names[bone_id][i].push_back("VFX" + std::to_string(bone_role_id));
    }

    for (auto& [bone_id, name_table] : bone_names) {
      auto& bone_ref = bone_refs.at(bone_id).get();
      std::string name;
      for (auto& [table_id, bone_names] : name_table) {
        if (table_id > 0) name += "|";

        for (std::size_t i = 0; i < bone_names.size(); i++) {
          if (i > 0) name += "_";

          name += bone_names.at(i);
        }
      }

      bone_ref.name = name;
    }
  }
}

std::vector<std::map<BoneTarget, BoneIndex>> GenerateBoneTargetTables(const SSkeleton& skeleton) {
  auto bone_refs = skeleton.GetBoneRefs();

  std::vector<std::map<BoneTarget, BoneIndex>> bone_body_tables;  // max size is 2

  for (std::size_t i = 0; i < bone_refs.size(); i++) {
    std::string bone_name = bone_refs.at(i).get().name;

    auto table = utl::string::Split(bone_name, "|");

    for (std::size_t j = 0; j < std::min<std::size_t>(table.size(), 2U); j++) {
      auto names = utl::string::Split(table.at(j), "_");

      for (std::size_t n = 0; n < names.size(); n++) {
        auto name = utl::string::ToLower(names.at(n));

        if (utl::string::StartsWith(name, "vfx")) {
          u16 bone_role = (u16)std::min<std::size_t>(std::stoul(name.substr(3)), bone_target_names.size() - 1);

          if (bone_body_tables.size() < j + 1) bone_body_tables.resize(j + 1);

          bone_body_tables.at(j).insert({(u16)bone_role, (u16)i});
          continue;
        }

        // Blender postfix
        if (utl::string::EndsWith(name, "_l") || utl::string::EndsWith(name, "_r") ||
            utl::string::EndsWith(name, ".l") || utl::string::EndsWith(name, ".r") ||
            utl::string::EndsWith(name, "-l") || utl::string::EndsWith(name, "-r"))
          name = name.substr(0, name.size() - 2);

        auto itr = std::find_if(bone_target_names.begin(), bone_target_names.end(),
                                [&name](const std::string_view a) { return utl::string::ToLower(a) == name; });

        if (itr != bone_target_names.end()) {
          u16 bone_role = static_cast<u16>(itr - bone_target_names.begin());

          if (bone_body_tables.size() < j + 1) bone_body_tables.resize(j + 1);

          bone_body_tables.at(j).insert({(u16)bone_role, (u16)i});
        }
      }
    }
  }

  // If an entry is missing, insert it (if there's kRoot and kSpine in the table
  // there must be kHips as well):

  const std::vector<i16> bone_parent{-1,         kRoot,          kHips,         kSpine,        kSpine1,
                                     kNeck,      kSpine1,        kLeftShoulder, kLeftArm,      kLeftForeArm,
                                     kSpine1,    kRightShoulder, kRightArm,     kRightForeArm, kHips,
                                     kLeftUpLeg, kLeftLeg,       kHips,         kRightUpLeg,   kRightLeg};

  for (auto& bone_table : bone_body_tables) {
    if (bone_table.empty()) continue;

    std::size_t bone_role_count = 1 + bone_table.rbegin()->first;

    for (std::size_t j = 0; j < bone_role_count && j < bone_parent.size(); j++) {
      i16 bone_index = j;

      while ((bone_index != -1) && bone_table.count(bone_index) == 0) {
        bone_index = bone_parent.at(bone_index);
      }

      if (bone_table.count(bone_index) > 0) bone_table.insert({(u16)j, (u16)bone_table.at(bone_index)});
    }

    for (std::size_t j = 0; j < bone_role_count; j++) {
      bone_table.insert({(u16)j, (u16)0});
    }
  }

  return bone_body_tables;
}

SSkeleton Convert(const std::vector<Bone>& bones) {
  auto ConvertBone = [](SBone& sbone, const Bone& bone, i16 i) {
    sbone.scale = bone.scale;

    sbone.rotation = glm::quat(glm::radians(bone.rotation));

    sbone.translation = bone.translation;

    sbone.name = std::to_string(i);
  };

  i16 i = 0;

  std::function<void(SBone&, i16)> Convert;
  Convert = [&](SBone& root, i16 bound_bone_id) {
    while ((std::size_t)i < bones.size()) {
      const Bone& bone = bones.at(i);
      assert(bone.parent_index < i);
      if (bone.parent_index < bound_bone_id || bone.parent_index >= i) {
        return;
      }

      auto& child = root.children.emplace_back();

      ConvertBone(child, bone, i);

      Convert(child, i++);
    }
  };

  SSkeleton sskeleton;

  while ((std::size_t)i < bones.size()) {
    auto& root = sskeleton.roots.emplace_back();
    const Bone& bone = bones.at(i);
    ConvertBone(root, bone, i);

    Convert(root, i++);
  }

  return sskeleton;
}

std::map<BoneIndex, glm::mat4> GenerateInverseMatrixBoneTable(const SModel& smodel) {
  std::map<BoneIndex, glm::mat4> inverse_matrix_bone_table;

  std::set<BoneIndex> used_bones;

  for (auto& smesh : smodel.meshes) {
    for (auto& vertex : smesh.vertices) {
      NNL_EXPECTS_DBG(!vertex.weights.empty());
      for (const auto& weight : vertex.weights) {
        used_bones.insert(weight.first);
      }
    }
  }

  const auto bone_refs = smodel.skeleton.GetBoneRefs();

  for (BoneIndex bone_index : used_bones) {
    NNL_EXPECTS(bone_index < bone_refs.size());

    const auto& bone_ref = bone_refs[bone_index].get();

    inverse_matrix_bone_table.insert({bone_index, bone_ref.inverse});
  }

  return inverse_matrix_bone_table;
}

UVAnimation Convert(const SUVChannel& suv_anim_channel) {
  auto& [interpolation, keys] = suv_anim_channel;

  UVAnimation uv_anim;

  if (keys.size() > 1) {
    bool all_equal = true;

    auto& first_key = keys.at(0);

    for (std::size_t i = 1; i < keys.size(); i++) {
      auto& key = keys.at(i);

      // Blender currently always exports with linear
      // interpolation!

      // Detect an appropriate uv shift mode:
      if (!utl::math::IsApproxEqual(key.value, first_key.value)) {
        if (interpolation == SInterpolationMode::kConstant ||
            (key.time - 1 == keys.at(i - 1).time && i != 1 && all_equal)) {
          uv_anim.animation_mode = UVAnimationMode::kFramedShift;

          uv_anim.u_shift = key.value.x - first_key.value.x;
          uv_anim.v_shift = key.value.y - first_key.value.y;

          uv_anim.num_frames = key.time - first_key.time;
          break;
        }

        if (utl::math::IsApproxEqual(key.value, -1.0f * keys.at(1).value) && i != 1) {
          uv_anim.animation_mode = UVAnimationMode::kRangedShift;

          uv_anim.num_frames = (key.time - keys.at(1).time) * 2;

          double shift = (2.0 * M_PI) / (double)uv_anim.num_frames;

          uv_anim.u_shift = key.value.x / glm::cos((double)key.time * shift);
          uv_anim.v_shift = key.value.y / glm::sin((double)key.time * shift);
          break;
        }

        uv_anim.animation_mode = UVAnimationMode::kContinuousShift;

        uv_anim.u_shift = (key.value.x - first_key.value.x) / (key.time - first_key.time);
        uv_anim.v_shift = (key.value.y - first_key.value.y) / (key.time - first_key.time);

        uv_anim.num_frames = 0;

        all_equal = false;
      }
    }
  }

  if (std::abs(uv_anim.u_shift) < 0.00005 && std::abs(uv_anim.v_shift) < 0.00005) {
    uv_anim.animation_mode = UVAnimationMode::kNone;
    uv_anim.u_shift = 0.0f;
    uv_anim.v_shift = 0.0f;
    uv_anim.num_frames = 0;
  }

  return uv_anim;
}

SUVChannel Convert(const UVAnimation& uv_anim) {
  SUVChannel suv_channel;

  auto& [interpolation, schannel] = suv_channel;

#ifndef NNL_UVANIM_MAX_DUR
#define NNL_UVANIM_MAX_DUR 1000
#endif

  constexpr std::size_t desired_duration = NNL_UVANIM_MAX_DUR;

  static_assert(desired_duration != 0, "");

  if (uv_anim.animation_mode == UVAnimationMode::kNone || (uv_anim.u_shift == 0.0f && uv_anim.v_shift == 0.0f)) {
    schannel.push_back({0, glm::vec2(0.0f)});
    // A workaround for Blender to avoid channel deletion
    schannel.push_back({desired_duration / 2, glm::vec2(0.025f)});
  }

  else if (uv_anim.animation_mode == UVAnimationMode::kContinuousShift || uv_anim.num_frames == 0) {
    interpolation = SInterpolationMode::kLinear;

    // Find the frame where looping happens (u and v reach an integer value)
    u16 loop_frame =
        std::clamp<u16>(std::round(10000.0f / (float)std::max(std::gcd((int)std::round(uv_anim.u_shift * 10000.0f),
                                                                       (int)std::round(uv_anim.v_shift * 10000.0f)),
                                                              1)),
                        1, desired_duration);

    u16 max_frame = loop_frame * (desired_duration / loop_frame);

    schannel.push_back({0, glm::vec2(0.0f)});

    schannel.push_back({max_frame, glm::vec2((float)max_frame * uv_anim.u_shift, (float)max_frame * uv_anim.v_shift)});

  }

  else if (uv_anim.animation_mode == UVAnimationMode::kFramedShift) {
    interpolation = SInterpolationMode::kConstant;

    std::size_t loop_frame = std::clamp<std::size_t>(
        uv_anim.num_frames *
            (std::size_t)std::round(10000.0f / (float)std::gcd((int)std::round(uv_anim.u_shift * 10000.0f),
                                                               (int)std::round(uv_anim.v_shift * 10000.0f))),
        1, desired_duration);

    std::size_t max_frame = loop_frame * (desired_duration / loop_frame);

    for (std::size_t i = 0; i <= max_frame; i += uv_anim.num_frames) {
      schannel.push_back({static_cast<u16>(i), glm::vec2(uv_anim.u_shift * (i / uv_anim.num_frames),
                                                         uv_anim.v_shift * (i / uv_anim.num_frames))});
    }
  } else if (uv_anim.animation_mode == UVAnimationMode::kRangedShift) {
    interpolation = SInterpolationMode::kLinear;

    double shift = 0.0f;

    for (std::size_t i = 0; i <= uv_anim.num_frames * std::max<std::size_t>(desired_duration / uv_anim.num_frames, 1);
         i++) {
      schannel.push_back({(u16)i, glm::vec2(uv_anim.u_shift * glm::cos(shift), uv_anim.v_shift * glm::sin(shift))});

      shift += ((M_PI * 2) / (double)uv_anim.num_frames);
    }
  }
  return suv_channel;
}

Material Convert(const SMaterial& smat) {
  Material mat;
  NNL_EXPECTS_DBG(smat.texture_id >= -1);
  mat.texture_id = smat.texture_id;

  mat.vfx_group_id = smat.vfx_group_id;

  mat.ambient = (utl::color::FloatToInt(smat.opacity) << 24) | (0x00'FF'FF'FF & utl::color::FloatToInt(smat.ambient));

  mat.diffuse = utl::color::FloatToInt(smat.diffuse);
  mat.specular = utl::color::FloatToInt(smat.specular);
  mat.emissive = utl::color::FloatToInt(smat.emissive);
  mat.specular_power = smat.specular_power;

  if ((smat.alpha_mode & SBlendMode::kClip) != SBlendMode::kOpaque) mat.features = MaterialFeatures::kAlphaClip;

  if ((smat.alpha_mode & SBlendMode::kAlpha) != SBlendMode::kOpaque)
    mat.features = MaterialFeatures::kNoDepthWriteDefer;

  if ((smat.alpha_mode & SBlendMode::kAdd) != SBlendMode::kOpaque) {
    mat.features &= (~MaterialFeatures::kAlphaClip);
    mat.features |= MaterialFeatures::kAdditiveBlending;
  }

  if ((smat.alpha_mode & SBlendMode::kSub) != SBlendMode::kOpaque) {
    // Disable some flags first since they won't have any effect anyway
    mat.features &= (~MaterialFeatures::kAlphaClip);
    mat.features &= (~MaterialFeatures::kAdditiveBlending);
    mat.features |= MaterialFeatures::kSubtractiveBlending;
  }

  if (smat.projection_mode == STextureProjection::kEnvironment) mat.features |= MaterialFeatures::kEnvironmentMapping;

  if (smat.projection_mode == STextureProjection::kMatrix) mat.features |= MaterialFeatures::kProjectionMapping;

  if (smat.lit) mat.features |= MaterialFeatures::kLit;

  return mat;
}

SMaterial Convert(const Material& mat) {
  SMaterial smat;

  smat.name = "_d_" + utl::string::IntToHex(mat.diffuse) + "_a_" + utl::string::IntToHex(mat.ambient) + "_s_" +
              utl::string::IntToHex(mat.specular) + "_e_" + utl::string::IntToHex(mat.emissive) + "_f_" +
              utl::string::IntToHex(utl::data::as_int(mat.features)).substr(1, 3) + "_u_" +
              utl::string::IntToHex(mat.uv_animation_id) + "_x_" + utl::string::IntToHex(mat.vfx_group_id);

  smat.texture_id = mat.texture_id;
  smat.diffuse = utl::color::IntToFloat(mat.diffuse);
  smat.ambient = utl::color::IntToFloat(mat.ambient);

  smat.specular = utl::color::IntToFloat(mat.specular);
  smat.specular_power = mat.specular_power;
  smat.emissive = utl::color::IntToFloat(mat.emissive);

  smat.opacity = (mat.ambient >> 24) / 255.0f;

  smat.vfx_group_id = mat.vfx_group_id;
  // Ordered by precedence:
  if ((mat.features & MaterialFeatures::kAlphaClip) != MaterialFeatures::kNone) smat.alpha_mode = SBlendMode::kClip;

  if ((mat.features & MaterialFeatures::kNoDepthWriteDefer) != MaterialFeatures::kNone)
    smat.alpha_mode = SBlendMode::kAlpha;

  if ((mat.features & MaterialFeatures::kAdditiveBlending) != MaterialFeatures::kNone) {
    smat.alpha_mode &= (~SBlendMode::kClip);
    smat.alpha_mode |= SBlendMode::kAdd;
  }

  if ((mat.features & MaterialFeatures::kSubtractiveBlending) != MaterialFeatures::kNone) {
    smat.alpha_mode &= (~SBlendMode::kClip);
    smat.alpha_mode &= (~SBlendMode::kAdd);
    smat.alpha_mode |= SBlendMode::kSub;
  }

  if ((mat.features & MaterialFeatures::kEnvironmentMapping) != MaterialFeatures::kNone)
    smat.projection_mode = STextureProjection::kEnvironment;

  if ((mat.features & MaterialFeatures::kProjectionMapping) != MaterialFeatures::kNone)
    smat.projection_mode = STextureProjection::kMatrix;

  smat.lit = ((mat.features & MaterialFeatures::kLit) != MaterialFeatures::kNone);

  return smat;
}

Model Convert_(SModel&& smodel, const std::vector<ConvertParam>& mesh_params, bool move_with_root) {
  NNL_EXPECTS(mesh_params.size() == smodel.meshes.size());
  NNL_EXPECTS(!smodel.skeleton.roots.empty());
  NNL_EXPECTS(!smodel.meshes.empty());

  Model model;
  model.move_with_root = move_with_root;
  model.bone_target_tables = GenerateBoneTargetTables(smodel.skeleton);

  bool normalize_pos = false;
  bool normalize_uv = false;

  for (auto& sattachment : smodel.attachments) {
    Attachment& attachment = model.attachments.emplace_back();

    attachment.attachment_id = sattachment.id;

    attachment.bone_id = sattachment.bone;

    attachment.transform = sattachment.GetTransform();
  }

  for (const auto& smat : smodel.materials) {
    model.materials.push_back(Convert(smat));
  }

  model.uv_animations.resize(1 + smodel.uv_animations.size());

  for (std::size_t i = 0; i < smodel.uv_animations.size(); i++) {
    const auto& suv_anim = smodel.uv_animations.at(i);

    std::size_t uv_anim_id = -1;

    // Same SUVAnimation may target multiple materials and use multiple channels;
    // Those channels should not lead to multiple duplicate UVAnimations:
    std::unordered_map<UVAnimation, std::size_t, Hash> inserted_anims;

    for (const auto& [mat_id, mat_anim] : suv_anim.translation_channels) {
      UVAnimation uv_anim = Convert(mat_anim);

      if (inserted_anims.empty()) {
        uv_anim_id = i + 1;
        model.uv_animations.at(i + 1) = uv_anim;
        inserted_anims[uv_anim] = i + 1;
      } else {
        auto itr_anim = inserted_anims.find(uv_anim);

        if (itr_anim == inserted_anims.end()) {
          inserted_anims[uv_anim] = model.uv_animations.size();
          uv_anim_id = model.uv_animations.size();
          model.uv_animations.push_back(uv_anim);

        } else {
          uv_anim_id = itr_anim->second;
        }
      }

      model.materials.at(mat_id).uv_animation_id = uv_anim_id;
    }
  }

  for (std::size_t i = 0; i < smodel.meshes.size(); i++) {
    auto& smesh = smodel.meshes.at(i);

    const ConvertParam& param = mesh_params.at(i);

    if (smesh.uses_color) model.materials.at(smesh.material_id).features |= MaterialFeatures::kVertexColors;

    if (param.compress_lvl != CompLvl::kNone || vertexde::IsFixedPosition(param.force_vertex_format))
      normalize_pos = true;

    if (param.compress_lvl != CompLvl::kNone || vertexde::IsFixedUV(param.force_vertex_format)) normalize_uv = true;
  }

  std::vector<std::pair<glm::vec2, glm::vec2>> uv_transforms;

  if (normalize_pos) smodel.Normalize();

  if (normalize_uv) uv_transforms = smodel.NormalizeUV();

  model.inverse_matrix_bone_table = GenerateInverseMatrixBoneTable(smodel);

  model.skeleton = Convert(smodel.skeleton);

  model.texture_swaps.resize(smodel.material_variants.size());

  for (std::size_t i = 0; i < smodel.meshes.size(); i++) {
    auto& smesh = smodel.meshes.at(i);

    const ConvertParam& param = mesh_params.at(i);

    for (auto& [new_mat_id, variants] : smesh.material_variants) {
      for (auto variant : variants) {
        TextureSwap& swap = model.texture_swaps.at(variant);

        int original_id = smodel.materials.at(smesh.material_id).texture_id;
        int new_id = smodel.materials.at(new_mat_id).texture_id;

        if (original_id != -1 && new_id != -1) {
          swap.original_texture_id = (i16)original_id;
          swap.new_texture_id = (i16)new_id;
        }
      }
    }

    if (model.mesh_groups.size() < smesh.mesh_group + 1) model.mesh_groups.resize(smesh.mesh_group + 1);

    auto& mesh_group = model.mesh_groups.at(smesh.mesh_group);

    if (normalize_uv) {
      auto& uv_transform = uv_transforms.at(smesh.mesh_group);
      mesh_group.u_offset = uv_transform.first.x;
      mesh_group.v_offset = uv_transform.first.y;
      mesh_group.u_scale = uv_transform.second.x;
      mesh_group.v_scale = uv_transform.second.y;
    }

    auto mesh = Convert(std::move(smesh), param);

    if (param.use_bbox) {
      auto bone_refs = smodel.skeleton.GetBoneRefs();

      // Bake inverse transformation of the bone since
      // it's not applied by the game engine
      auto transform = bone_refs.at(mesh.bbox.bone_index).get().inverse;

      mesh.bbox.min = transform * glm::vec4(mesh.bbox.min, 0.0f);
      mesh.bbox.max = transform * glm::vec4(mesh.bbox.max, 0.0f);
    }

    mesh_group.meshes.push_back(std::move(mesh));
  }

  smodel = {};

  return model;
}

Model Convert(SModel&& smodel, const ConvertParam& mesh_params, bool move_with_root) {
  std::vector<ConvertParam> params(smodel.meshes.size(), mesh_params);
  return Convert_(std::move(smodel), params, move_with_root);
}

Model Convert(SModel&& smodel, const std::vector<ConvertParam>& mesh_params, bool move_with_root) {
  return Convert_(std::move(smodel), mesh_params, move_with_root);
}

SModel Convert(const Model& model) {
  SModel smodel;

  smodel.skeleton = Convert(model.skeleton);

  for (auto& attachment : model.attachments) {
    SAttachment& sattachment = smodel.attachments.emplace_back();

    sattachment.id = attachment.attachment_id;
    sattachment.bone = attachment.bone_id;
    sattachment.SetTransform(attachment.transform);
  }

  auto bone_refs = smodel.skeleton.GetBoneRefs();

  SetBoneNames(smodel.skeleton, model.bone_target_tables, model.move_with_root);

  // Inverse bind matrices usually include a scaling matrix

  for (auto [bone_id, inverse_matrix] : model.inverse_matrix_bone_table) {
    bone_refs.at(bone_id).get().inverse = inverse_matrix;
  };

  std::vector<std::map<std::size_t, std::size_t>> mappings(model.materials.size());

  smodel.material_variants.resize(model.texture_swaps.size());

  smodel.materials.resize(model.materials.size());

  // Ignore the default uv anim
  if (model.uv_animations.size() > 1) smodel.uv_animations.resize(model.uv_animations.size() - 1);

  for (std::size_t i = 0; i < model.materials.size(); i++) {
    const Material& mat = model.materials.at(i);

    SMaterial& smat = smodel.materials.at(i);

    smat = Convert(mat);

    smat.name = utl::string::PrependZero(i, 3) + smat.name;

    const bool is_black_outline = mat.vfx_group_id == 1 && mat.texture_id == -1 && model.move_with_root;

    if (is_black_outline) smat.ambient = glm::vec3(0.0f);

    for (std::size_t j = 0; j < model.texture_swaps.size(); j++) {
      const TextureSwap& swap = model.texture_swaps.at(j);

      smodel.material_variants.at(j) = utl::string::PrependZero(j) + "_texture_swap_t_" +
                                       utl::string::PrependZero(swap.original_texture_id) + "_" +
                                       utl::string::PrependZero(swap.new_texture_id);

      if (mat.texture_id == swap.original_texture_id) {
        std::size_t new_smat_id = smodel.materials.size();

        mappings.at(i)[j] = new_smat_id;

        auto new_smat = smodel.materials.at(i);

        new_smat.name = utl::string::PrependZero(i, 3) + "_variant_t_" + utl::string::PrependZero(swap.new_texture_id);

        new_smat.texture_id = swap.new_texture_id;

        smodel.materials.push_back(new_smat);
      }
    }

    if (mat.uv_animation_id != 0) {
      const UVAnimation& uv_anim = model.uv_animations.at(mat.uv_animation_id);

      auto& suv_anim = smodel.uv_animations.at(mat.uv_animation_id - 1);

      suv_anim.translation_channels.insert({i, Convert(uv_anim)});

      suv_anim.name = "uv_anim_" + utl::string::PrependZero(mat.uv_animation_id);

      if (uv_anim.animation_mode == UVAnimationMode::kNone || (uv_anim.u_shift == 0.0f && uv_anim.v_shift == 0.0f)) {
        suv_anim.name += "_target_holder";
      }

      else if (uv_anim.animation_mode == UVAnimationMode::kContinuousShift || uv_anim.num_frames == 0) {
        suv_anim.name += "_continuous_" + utl::string::FloatToString(uv_anim.u_shift) + "," +
                         utl::string::FloatToString(uv_anim.v_shift);

      }

      else if (uv_anim.animation_mode == UVAnimationMode::kFramedShift) {
        suv_anim.name += "_framed_" + std::to_string(uv_anim.num_frames) + "_" +
                         utl::string::FloatToString(uv_anim.u_shift) + "," +
                         utl::string::FloatToString(uv_anim.v_shift);

      } else if (uv_anim.animation_mode == UVAnimationMode::kRangedShift) {
        suv_anim.name += "_ranged_" + std::to_string(uv_anim.num_frames) + "_" +
                         utl::string::FloatToString(uv_anim.u_shift) + "," +
                         utl::string::FloatToString(uv_anim.v_shift);
      }
    }
  }

  for (std::size_t i = 0; i < model.mesh_groups.size(); i++) {
    const auto& mesh_group = model.mesh_groups.at(i);

    for (std::size_t k = 0; k < mesh_group.meshes.size(); k++) {
      const auto& mesh = mesh_group.meshes.at(k);

      SMesh smesh = Convert(mesh);

      smesh.mesh_group = i;

      smesh.name = "meshg_" + utl::string::PrependZero(i) + "_m_" + utl::string::PrependZero(k, 3) + smesh.name;

      SMaterial& smat = smodel.materials.at(smesh.material_id);

      const Material& mat = model.materials.at(smesh.material_id);

      // Vertex colors won't be used without this flag
      if (smat.lit && (mat.features & MaterialFeatures::kVertexColors) == MaterialFeatures::kNone)
        smesh.uses_color = false;

      if (smesh.uses_color) {
        smat.diffuse = glm::vec3(1.0f);
        smat.ambient = glm::vec3(1.0f);
        smat.opacity = 1.0f;
      }

      auto& mapping = mappings.at(smesh.material_id);

      for (std::size_t t = 0; t < model.texture_swaps.size(); t++) {
        if (mapping.count(t)) {
          smesh.material_variants[mapping.at(t)].push_back(t);
        }
      }

      if (smesh.uses_uv) {
        glm::mat3 transform{1.0f};

        transform[0][0] = mesh_group.u_scale;
        transform[1][1] = mesh_group.v_scale;
        transform[2][0] = mesh_group.u_offset;
        transform[2][1] = mesh_group.v_offset;

        smesh.TransformUV(transform);
      }

      smodel.meshes.push_back(std::move(smesh));
    }
  }

  smodel.TryBakeBindShape();

  return smodel;
}

bool IsOfType_(Reader& f) {
  using namespace raw;

  f.Seek(0);

  if (f.Len() < sizeof(RHeader)) return false;

  auto rheader = f.ReadLE<RHeader>();

  if (rheader.magic_bytes != kMagicBytes) return false;

  if (rheader.offset_bones < sizeof(RHeader)) return false;

  if (rheader.offset_bones % sizeof(float) != 0) return false;

  if (rheader.offset_inverse_matrices < sizeof(RHeader)) return false;

  if (rheader.offset_inverse_matrices % sizeof(Vec4<float>) != 0) return false;

  return true;
}

bool IsOfType(Reader& f) { return IsOfType_(f); }

bool IsOfType(BufferView buffer) { return IsOfType_(buffer); }

bool IsOfType(const std::filesystem::path& path) {
  FileReader f{path};
  return IsOfType_(f);
}

namespace raw {

std::vector<Bone> Convert(const std::vector<raw::RBone>& rbones) {
  std::vector<Bone> bones(rbones.size());

  for (std::size_t i = 0; i < rbones.size(); i++) {
    Bone& root = bones[i];

    const RBone& rbone = rbones[i];

    root.parent_index = rbone.parent_index;
    root.scale = glm::vec3(rbone.scale.x, rbone.scale.y, rbone.scale.z);
    root.translation = glm::vec3(rbone.translation.x, rbone.translation.y, rbone.translation.z);
    auto rotationS16 = rbone.rotation;

    root.rotation =
        glm::vec3(utl::math::FixedToFloat<i16, 1>(rotationS16.x), utl::math::FixedToFloat<i16, 1>(rotationS16.y),
                  utl::math::FixedToFloat<i16, 1>(rotationS16.z));
    root.rotation *= 90.0f;
  }

  return bones;
}

Model Convert(RModel&& rmodel) {
  Model model;

  model.move_with_root = rmodel.header.move_with_root;
  model.skeleton = Convert(rmodel.bones);

  // Inverse matrices
  for (std::size_t i = 0; i < rmodel.inverse_matrix_bone_table.size(); i++) {
    u16 bone_id = rmodel.inverse_matrix_bone_table.at(i);
    glm::mat4 inverse_mat(1.0f);
    std::memcpy(glm::value_ptr(inverse_mat), &(rmodel.inverse_matrices.at(i)), sizeof(glm::mat4));
    model.inverse_matrix_bone_table.insert({bone_id, inverse_mat});
  }

  // Bone target tables
  for (auto& rbone_role_table : rmodel.bone_role_table.entries) {
    auto& bone_role_table = model.bone_target_tables.emplace_back();
    for (const auto& body_part : rbone_role_table) {
      bone_role_table.insert(std::make_pair(body_part.role_id, body_part.bone_index));
    }
  }

  // Texture swap configs
  for (const auto& texture_swap : rmodel.texture_swaps) {
    model.texture_swaps.push_back({texture_swap.original_texture_id, texture_swap.new_texture_id});
  }

  // Attached object matrices
  for (std::size_t i = 0; i < rmodel.attachments.size(); i++) {
    const auto& obj_mat_conf = rmodel.attachments.at(i);
    glm::mat4 object_matrix(1.0f);
    std::memcpy(glm::value_ptr(object_matrix), &(rmodel.attachment_matrices.at(i)), sizeof(glm::mat4));
    model.attachments.push_back({obj_mat_conf.bone_id, obj_mat_conf.attachment_id, object_matrix});
  }

  // Texture animations
  for (const auto& uv_config : rmodel.uv_animations) {
    model.uv_animations.push_back({static_cast<UVAnimationMode>(uv_config.animation_mode), uv_config.num_frames,
                                   uv_config.u_shift, uv_config.v_shift});
  }

  u16 max_mat_id = 0;
  // Mesh groups
  for (const auto& rmesh_group : rmodel.mesh_groups) {
    model.mesh_groups.push_back(MeshGroup());
    auto& mesh_group = model.mesh_groups.back();

    mesh_group.u_offset = rmesh_group.u_offset;
    mesh_group.v_offset = rmesh_group.v_offset;
    mesh_group.u_scale = rmesh_group.u_scale;
    mesh_group.v_scale = rmesh_group.v_scale;

    for (u16 i = 0; i < rmesh_group.num_meshes; i++) {
      auto& rmesh = rmodel.meshes.at(rmesh_group.offset_meshes).at(i);
      mesh_group.meshes.push_back(Mesh());
      auto& mesh = mesh_group.meshes.back();

      if (rmesh.material_id > max_mat_id) max_mat_id = rmesh.material_id;

      mesh.use_bbox = rmesh.use_bbox;

      mesh.use_skinning = (bool)(rmesh.material_features & (1 << 7));

      if (rmesh.use_bbox) {
        // BBox
        const auto& rbbox = rmodel.bboxes.at(rmesh.offset_bbox);
        mesh.bbox.bone_index = rbbox.bone_index;

        mesh.bbox.min = glm::vec3(rbbox.vertices[0].x, rbbox.vertices[0].y, rbbox.vertices[0].z);
        mesh.bbox.max = glm::vec3(rbbox.vertices[7].x, rbbox.vertices[7].y, rbbox.vertices[7].z);
      }
      // Submeshes
      for (u16 j = 0; j < rmesh.num_submeshes; j++) {
        const auto& rsubmesh = rmodel.submeshes.at(rmesh.offset_submeshes).at(j);

        mesh.submeshes.push_back(SubMesh());
        auto& submesh = mesh.submeshes.back();
        submesh.num_bones = rsubmesh.num_bones;
        submesh.vertex_format = rsubmesh.vertex_format;

        for (u16 i = 0; i < std::max((u32)submesh.num_bones, 1u); i++) {
          submesh.bone_indices[i] = rmodel.inverse_matrix_bone_table.at(rsubmesh.inv_mat_bone_indices[i]);
        }

        for (u16 k = 0; k < rsubmesh.num_primitives; k++) {
          const auto& rprimitive = rmodel.primitives.at(rsubmesh.offset_primitives).at(k);

          auto& primitive = submesh.primitives.emplace_back();

          primitive.primitive_type = static_cast<PrimitiveType>(rprimitive.primitive_type);
          primitive.num_elements = rprimitive.num_elements;
          primitive.vertex_index_buffer = std::move(rmodel.vertex_buffers.at(rprimitive.offset_buffer));
        }

        submesh.display_list = std::move(rmodel.display_lists.at(rsubmesh.offset_display_list));

        if (rsubmesh.offset_indexed_vertex_buffer != 0) {
          submesh.indexed_vertex_buffer =
              std::move(rmodel.indexed_vertex_buffers.at(rsubmesh.offset_indexed_vertex_buffer));
        }
      }
    }
  }

  // In the raw binary format, there is no separate array for materials.
  // Instead, each mesh contains a copy of its material, and the material_id is
  // used to determine if the next mesh shares the same material. If it does,
  // the game can avoid unnecessary updates to the rendering context. However,
  // due to a previous bug in the library, a few invalid assets may have been created.
  // This attempts to fix it:

  std::map<u16, Material> id_mat;  // An id leads to a unique material,
  std::map<Material, u16> mat_id;  // the same material must lead to the same id.
  std::size_t prev_id = -1;

  for (std::size_t i = 0; i < model.mesh_groups.size(); i++) {
    auto& mesh_group = model.mesh_groups.at(i);
    auto& rmesh_group = rmodel.mesh_groups.at(i);
    prev_id = -1;
    for (std::size_t j = 0; j < mesh_group.meshes.size(); j++) {
      auto& mesh = mesh_group.meshes.at(j);
      auto& rmesh = rmodel.meshes.at(rmesh_group.offset_meshes).at(j);
      Material mat;
      mat.ambient = rmesh.ambient;
      mat.diffuse = rmesh.diffuse;
      mat.emissive = rmesh.emissive;
      mat.specular = rmesh.specular;
      mat.specular_power = rmesh.specular_power;
      mat.features = static_cast<MaterialFeatures>(rmesh.material_features & ~(1 << 7));
      mat.texture_id = rmesh.texture_id;

      mat.uv_animation_id = rmesh.uv_animation_id;

      mat.vfx_group_id = rmesh.vfx_group_id;

      auto itr_mat = id_mat.find(rmesh.material_id);

      if (rmesh.material_id == prev_id) {
        mesh.material_id = prev_id;
      } else if (itr_mat == id_mat.end()) {
        id_mat.insert({(u16)rmesh.material_id, mat});
        mat_id.insert({mat, (u16)rmesh.material_id});
        mesh.material_id = rmesh.material_id;

      } else {
        auto itr_id = mat_id.find(mat);

        if (itr_id != mat_id.end()) {
          mesh.material_id = itr_id->second == rmesh.material_id ? rmesh.material_id  // OK
                                                                 : itr_id->second;    // fix:
        } else {
          max_mat_id += 1;
          id_mat.insert({max_mat_id, mat});
          mat_id.insert({mat, max_mat_id});
          mesh.material_id = max_mat_id;
        }
      }

      prev_id = (mat.features & MaterialFeatures::kNoDepthWriteDefer) == MaterialFeatures::kNone ? mesh.material_id
                                                                                                 : (std::size_t)-1;
    }
  }

  if (!id_mat.empty()) model.materials.resize(id_mat.rbegin()->first + 1);

  for (auto& [id, mat] : id_mat) {
    model.materials.at(id) = mat;
  }
  rmodel = {};
  return model;
}

RModel Parse(Reader& f) {
  f.Seek(0);

  RModel model;

  NNL_TRY {
    model.header = f.ReadLE<RHeader>();

    if (model.header.magic_bytes != kMagicBytes) NNL_THROW(ParseError(NNL_ERMSG("invalid magic bytes")));

    // Bones
    f.Seek(model.header.offset_bones);

    model.bones = f.ReadArrayLE<RBone>(model.header.num_bones);

    // Matrices

    f.Seek(model.header.offset_inverse_matrices);

    model.inverse_matrices = f.ReadArrayLE<Mat4<f32>>(model.header.num_inverse_matrices);

    // Texture swap configs
    f.Seek(model.header.offset_texture_swaps);

    model.texture_swaps = f.ReadArrayLE<RTextureSwap>(model.header.num_texture_swaps);

    // Inverse matrix bone table
    f.Seek(model.header.offset_inverse_matrix_bone_table);

    model.inverse_matrix_bone_table = f.ReadArrayLE<u16>(model.header.num_inverse_matrices);

    // Bone target tables
    f.Seek(model.header.offset_bone_target_tables);

    model.bone_role_table.header = f.ReadArrayLE<RBoneTargetHeader>(model.header.num_bone_target_tables);

    for (u16 i = 0; i < model.header.num_bone_target_tables; i++) {
      auto& header = model.bone_role_table.header.at(i);

      auto& entries = model.bone_role_table.entries.emplace_back();

      f.Seek(header.offset);

      entries = f.ReadArrayLE<RBoneTargetEntry>(header.num_entries);
    }

    // Object attachment matrices
    f.Seek(model.header.offset_attachment_matrices);

    model.attachment_matrices = f.ReadArrayLE<Mat4<f32>>(model.header.num_attachment_matrices);

    f.Seek(model.header.offset_attachment_matrices + (model.header.num_attachment_matrices * sizeof(Mat4<f32>)));

    model.attachments = f.ReadArrayLE<RAttachment>(model.header.num_attachment_matrices);

    // UV animation configs
    f.Seek(model.header.offset_uv_animations);

    model.uv_animations = f.ReadArrayLE<RUVAnimation>(model.header.num_uv_animations);

    // Mesh groups
    f.Seek(model.header.offset_mesh_groups);

    model.mesh_groups = f.ReadArrayLE<RMeshGroup>(model.header.num_mesh_groups);

    for (const auto& mesh_group : model.mesh_groups) {
      f.Seek(mesh_group.offset_meshes);

      model.meshes[mesh_group.offset_meshes] = f.ReadArrayLE<RMesh>(mesh_group.num_meshes);

      // Submeshes
      for (const auto& mesh : model.meshes[mesh_group.offset_meshes]) {
        if (mesh.use_bbox) {
          f.Seek(mesh.offset_bbox);
          model.bboxes[mesh.offset_bbox] = f.ReadLE<RBBox>();
        }

        f.Seek(mesh.offset_submeshes);

        model.submeshes[mesh.offset_submeshes] = f.ReadArrayLE<RSubMesh>(mesh.num_submeshes);

        // Primitives and draw commands, indices
        for (const auto& submesh : model.submeshes[mesh.offset_submeshes]) {
          f.Seek(submesh.offset_primitives);

          model.primitives[submesh.offset_primitives] = f.ReadArrayLE<RPrimitive>(submesh.num_primitives);

          for (auto& primitive : model.primitives[submesh.offset_primitives]) {
            f.Seek(primitive.offset_buffer);
            model.vertex_buffers[primitive.offset_buffer] = f.ReadArrayLE<u8>(primitive.buffer_size);
          }

          f.Seek(submesh.offset_display_list);
          // Draw commands
          do {
            model.display_lists[submesh.offset_display_list].push_back(f.ReadLE<u32>());
          } while (submesh.offset_display_list != 0 &&
                   (0xFF'00'00'00 & model.display_lists[submesh.offset_display_list].back()) !=
                       utl::data::as_int(GeCmd::kReturn));

          model.display_lists[submesh.offset_display_list].pop_back();
          // Indexed vertex buffers
          if (submesh.offset_indexed_vertex_buffer != 0) {
            f.Seek(submesh.offset_indexed_vertex_buffer);

            u16 num_vertices = 0;

            for (auto& primitive : model.primitives[submesh.offset_primitives]) {
              auto& indices = model.vertex_buffers[primitive.offset_buffer];

              if (vertexde::Is8Indices(submesh.vertex_format)) {
                for (std::size_t i = 0; i < indices.size(); i++) {
                  if (indices.at(i) + 1 > num_vertices) num_vertices = indices.at(i) + 1;
                }
              } else {
                for (std::size_t i = 0; i < indices.size(); i += 2) {
                  u16 index = (indices.at(i + 1) << 8) | (indices.at(i));
                  if (index + 1 > num_vertices) num_vertices = index + 1;
                }
              }
            }

            auto layout = vertexde::GetLayout(submesh.vertex_format);

            std::size_t indexed_buffer_size =
                layout.vertex_size * vertexde::GetMorphNum(submesh.vertex_format) * num_vertices;
            model.indexed_vertex_buffers[submesh.offset_indexed_vertex_buffer] = f.ReadArrayLE<u8>(indexed_buffer_size);
          }
        }
      }
    }
  }
  NNL_CATCH(std::exception) { NNL_THROW(ParseError{NNL_ERMSG(e.what())}); }

  return model;
}

std::vector<raw::RBone> Convert(const std::vector<Bone>& bones) {
  using namespace raw;

  std::vector<RBone> rbones;

  for (const auto root : bones) {
    RBone rbone;

    NNL_EXPECTS(root.parent_index >= -1);
    NNL_EXPECTS(std::abs(root.rotation.x) <= 180.0f);
    NNL_EXPECTS(std::abs(root.rotation.y) <= 180.0f);
    NNL_EXPECTS(std::abs(root.rotation.z) <= 180.0f);

    NNL_EXPECTS(utl::math::IsFinite(root.translation));
    NNL_EXPECTS(utl::math::IsFinite(root.scale));

    rbone.parent_index = root.parent_index;

    rbone.rotation = {utl::math::FloatToFixed<i16, 1>(root.rotation.x / 90.0f),
                      utl::math::FloatToFixed<i16, 1>(root.rotation.y / 90.0f),
                      utl::math::FloatToFixed<i16, 1>(root.rotation.z / 90.0f)};

    rbone.scale = {root.scale.x, root.scale.y, root.scale.z};
    rbone.translation = {root.translation.x, root.translation.y, root.translation.z};
    rbones.push_back(rbone);
  }

  return rbones;
}

}  // namespace raw

Model Import_(Reader& f) { return raw::Convert(raw::Parse(f)); }

Model Import(Reader& f) { return Import_(f); }

Model Import(BufferView buffer) { return Import_(buffer); }

Model Import(const std::filesystem::path& path) {
  FileReader f{path};

  return Import_(f);
}

void Export_(const Model& model, Writer& f) {
  using namespace raw;
  f.Seek(0);

  NNL_EXPECTS(!model.skeleton.empty());  // At least 1 bone is required

  NNL_EXPECTS(model.inverse_matrix_bone_table.size() <= model.skeleton.size());

  RHeader header;
  header.num_mesh_groups = utl::data::narrow<u16>(model.mesh_groups.size());

  std::vector<RBone> rbones = raw::Convert(model.skeleton);
  header.num_bones = utl::data::narrow<u16>(rbones.size());
  header.num_bone_target_tables = utl::data::narrow<u16>(model.bone_target_tables.size());
  header.move_with_root = model.move_with_root;

  header.num_textures = [&model]() {
    int num = -1;

    for (auto& material : model.materials)
      if (material.texture_id > num) num = material.texture_id;

    for (auto& texture_swap : model.texture_swaps)
      if (texture_swap.new_texture_id > num) num = texture_swap.new_texture_id;

    return utl::data::narrow<u16>(num + 1);
  }();

  header.num_vfx_groups = [&model]() {
    std::size_t num = 0;

    for (auto& mat : model.materials)
      if (mat.vfx_group_id > num) num = mat.vfx_group_id;

    return utl::data::narrow<u16>(num + 1);
  }();

  header.num_inverse_matrices = utl::data::narrow<u16>(model.inverse_matrix_bone_table.size());

  header.num_texture_swaps = utl::data::narrow<u16>(model.texture_swaps.size());

  header.num_attachment_matrices = utl::data::narrow<u16>(model.attachments.size());

  header.num_uv_animations = utl::data::narrow<u16>(model.uv_animations.size());

  auto header_off = f.WriteLE(header);

  f.WriteLE<u32>(0x0);  // 0x44 weird padding, 0x48 for effects

  header_off->*& RHeader::offset_bones = utl::data::narrow<u32>(f.Tell());

  f.WriteArrayLE(rbones);

  // BBoxes are located after meshes of mesh groups
  f.AlignData(0x10, 0);

  header_off->*& RHeader::offset_inverse_matrices = utl::data::narrow<u32>(f.Tell());

  for (const auto& [bone_id, inverse_matrix] : model.inverse_matrix_bone_table) {
    NNL_EXPECTS(utl::math::IsFinite(inverse_matrix));
    f.WriteLE(inverse_matrix);
  }

  header_off->*& RHeader::offset_texture_swaps = utl::data::narrow<u32>(f.Tell());

  for (u16 i = 0; i < model.texture_swaps.size(); i++) {
    const auto& text_swap = model.texture_swaps.at(i);
    f.WriteLE(RTextureSwap{i, text_swap.original_texture_id, text_swap.new_texture_id});
  }

  header_off->*& RHeader::offset_inverse_matrix_bone_table = utl::data::narrow<u32>(f.Tell());

  for (const auto& [bone_id, inverse_matrix] : model.inverse_matrix_bone_table) {
    f.WriteLE(bone_id);
  }

  f.AlignData(0x4);

  {
    auto bone_body_header_off = f.MakeOffsetLE<RBoneTargetHeader>();

    header_off->*& RHeader::offset_bone_target_tables = utl::data::narrow<u32>(f.Tell());
    for (auto& bone_role_table : model.bone_target_tables) {
      f.WriteLE(RBoneTargetHeader{0, (u32)bone_role_table.size()});
    }

    for (auto& bone_role_table : model.bone_target_tables) {
      bone_body_header_off->*& RBoneTargetHeader::offset = utl::data::narrow<u32>(f.Tell());
      for (auto& [bone_target_id, bone_ind] : bone_role_table) {
        f.WriteLE(RBoneTargetEntry{bone_ind, bone_target_id});
      }
      ++bone_body_header_off;
    }
  }

  f.AlignData(0x10);

  header_off->*& RHeader::offset_attachment_matrices = utl::data::narrow<u32>(f.Tell());

  for (const auto& attachment : model.attachments) {
    NNL_EXPECTS(utl::math::IsFinite(attachment.transform));
    f.WriteLE(attachment.transform);
  }

  for (const auto& [bone_id, attachment_id, transform] : model.attachments) {
    NNL_EXPECTS(bone_id < model.skeleton.size());
    f.WriteLE(RAttachment{bone_id, attachment_id});
  }

  header_off->*& RHeader::offset_uv_animations = utl::data::narrow<u32>(f.Tell());

  for (const auto& uv_animation : model.uv_animations) {
    NNL_EXPECTS(uv_animation.animation_mode != UVAnimationMode::kRangedShift || uv_animation.num_frames > 0);

    NNL_EXPECTS(uv_animation.animation_mode == UVAnimationMode::kNone ||
                (std::abs(uv_animation.u_shift) <= 100.0f && std::abs(uv_animation.v_shift) <= 100.0f));

    RUVAnimation rconf;
    rconf.animation_mode = utl::data::as_int(uv_animation.animation_mode);
    rconf.u_shift = uv_animation.u_shift;
    rconf.v_shift = uv_animation.v_shift;
    rconf.num_frames = uv_animation.num_frames;
    f.WriteLE(rconf);
  }

  f.AlignData(0x4, 0);

  header_off->*& RHeader::offset_mesh_groups = utl::data::narrow<u32>(f.Tell());

  auto mesh_group_off = f.MakeOffsetLE<RMeshGroup>();

  for (const auto& mesh_group : model.mesh_groups) {
    NNL_EXPECTS(mesh_group.u_scale >= 0.0f);
    NNL_EXPECTS(mesh_group.v_scale >= 0.0f);
    NNL_EXPECTS(utl::math::IsFinite(mesh_group.u_scale));
    NNL_EXPECTS(utl::math::IsFinite(mesh_group.v_scale));
    NNL_EXPECTS(utl::math::IsFinite(mesh_group.u_offset));
    NNL_EXPECTS(utl::math::IsFinite(mesh_group.v_offset));
    RMeshGroup rmesh_group;

    rmesh_group.num_meshes = mesh_group.meshes.size();
    // rmesh_group->offset_meshes = offset;//  write it later
    rmesh_group.u_offset = mesh_group.u_offset;
    rmesh_group.v_offset = mesh_group.v_offset;
    rmesh_group.u_scale = mesh_group.u_scale;
    rmesh_group.v_scale = mesh_group.v_scale;

    f.WriteLE(rmesh_group);
  }

  for (const auto& mesh_group : model.mesh_groups)

  {
    auto mesh_off = f.MakeOffsetLE<RMesh>();

    auto mesh_off_copy = f.MakeOffsetLE<RMesh>();

    mesh_group_off->*& RMeshGroup::offset_meshes = utl::data::narrow<u32>(f.Tell());
    ++mesh_group_off;

    for (const auto& mesh : mesh_group.meshes) {
      RMesh rmesh;
      rmesh.num_submeshes = mesh.submeshes.size();
      auto& mat = model.materials.at(mesh.material_id);
      rmesh.ambient = mat.ambient;
      rmesh.diffuse = mat.diffuse;
      rmesh.emissive = mat.emissive;
      rmesh.specular = mat.specular;
      rmesh.specular_power = mat.specular_power;
      rmesh.material_features = utl::data::as_int(mat.features) | ((u16)mesh.use_skinning << 7);
      rmesh.use_bbox = mesh.use_bbox;
      rmesh.vfx_group_id = mat.vfx_group_id;

      NNL_EXPECTS(mat.uv_animation_id < model.uv_animations.size());

      rmesh.uv_animation_id = mat.uv_animation_id;

      NNL_EXPECTS(mat.texture_id >= -1);

      rmesh.texture_id = mat.texture_id;

      NNL_EXPECTS(mesh.material_id < model.materials.size());

      rmesh.material_id = mesh.material_id;
      f.WriteLE(rmesh);
    }

    // Write bboxes after meshes
    f.AlignData(0x10);

    for (const auto& mesh : mesh_group.meshes) {
      if (mesh.use_bbox) {
        NNL_EXPECTS(utl::math::IsFinite(mesh.bbox.min));
        NNL_EXPECTS(utl::math::IsFinite(mesh.bbox.max));
        NNL_EXPECTS(mesh.bbox.bone_index < model.skeleton.size());
        RBBox rbbox;

        rbbox.bone_index = mesh.bbox.bone_index;

        rbbox.vertices[0].x = mesh.bbox.min.x;
        rbbox.vertices[0].y = mesh.bbox.min.y;
        rbbox.vertices[0].z = mesh.bbox.min.z;
        rbbox.vertices[0].w = 1.0f;

        rbbox.vertices[1].x = mesh.bbox.max.x;
        rbbox.vertices[1].y = mesh.bbox.min.y;
        rbbox.vertices[1].z = mesh.bbox.min.z;
        rbbox.vertices[1].w = 1.0f;

        rbbox.vertices[2].x = mesh.bbox.min.x;
        rbbox.vertices[2].y = mesh.bbox.max.y;
        rbbox.vertices[2].z = mesh.bbox.min.z;
        rbbox.vertices[2].w = 1.0f;

        rbbox.vertices[3].x = mesh.bbox.max.x;
        rbbox.vertices[3].y = mesh.bbox.max.y;
        rbbox.vertices[3].z = mesh.bbox.min.z;
        rbbox.vertices[3].w = 1.0f;

        rbbox.vertices[4].x = mesh.bbox.min.x;
        rbbox.vertices[4].y = mesh.bbox.min.y;
        rbbox.vertices[4].z = mesh.bbox.max.z;
        rbbox.vertices[4].w = 1.0f;

        rbbox.vertices[5].x = mesh.bbox.max.x;
        rbbox.vertices[5].y = mesh.bbox.min.y;
        rbbox.vertices[5].z = mesh.bbox.max.z;
        rbbox.vertices[5].w = 1.0f;

        rbbox.vertices[6].x = mesh.bbox.min.x;
        rbbox.vertices[6].y = mesh.bbox.max.y;
        rbbox.vertices[6].z = mesh.bbox.max.z;
        rbbox.vertices[6].w = 1.0f;

        rbbox.vertices[7].x = mesh.bbox.max.x;
        rbbox.vertices[7].y = mesh.bbox.max.y;
        rbbox.vertices[7].z = mesh.bbox.max.z;
        rbbox.vertices[7].w = 1.0f;

        mesh_off_copy->*& RMesh::offset_bbox = utl::data::narrow<u32>(f.Tell());
        f.WriteLE(rbbox);
      }
      ++mesh_off_copy;
    }

    // If bbox is not used, this is set as offset to bbox

    auto submesh_off_copy = f.MakeOffsetLE<RSubMesh>();

    for (const auto& mesh : mesh_group.meshes) {
      auto submesh_off = f.MakeOffsetLE<RSubMesh>();

      if (!mesh.use_bbox) {
        mesh_off->*& RMesh::offset_bbox = utl::data::narrow_cast<u32>(submesh_off_copy);
      }
      mesh_off->*& RMesh::offset_submeshes = utl::data::narrow<u32>(f.Tell());
      ++mesh_off;

      for (const auto& submesh : mesh.submeshes) {
        RSubMesh rsubmesh;
        rsubmesh.vertex_format = submesh.vertex_format;
        NNL_EXPECTS(vertexde::HasPosition(submesh.vertex_format));

        NNL_EXPECTS((0xFF'00'00'00 & submesh.vertex_format) == 0);

        NNL_EXPECTS(submesh.display_list.empty() || submesh.display_list.size() >= 4 * submesh.primitives.size());

        NNL_EXPECTS(submesh.num_bones <= kMaxNumBonePerPrim);

        NNL_EXPECTS(mesh.use_skinning || submesh.num_bones > 0);

        for (u32 i = 0; i < std::max((u32)submesh.num_bones, 1u); i++) {
          auto bone_index = submesh.bone_indices[i];

          NNL_EXPECTS(bone_index < model.skeleton.size());

          auto bone_index_it = model.inverse_matrix_bone_table.find(bone_index);

          NNL_EXPECTS(bone_index_it != model.inverse_matrix_bone_table.end());

          rsubmesh.inv_mat_bone_indices[i] = std::distance(model.inverse_matrix_bone_table.cbegin(), bone_index_it);
        }

        rsubmesh.num_bones = submesh.num_bones;
        rsubmesh.num_primitives = utl::data::narrow<u16>(submesh.primitives.size());
        f.WriteLE(rsubmesh);
      }

      for (const auto& submesh : mesh.submeshes) {
        auto primitive_off = f.MakeOffsetLE<RPrimitive>();

        const bool has_indices = vertexde::IsIndexed(submesh.vertex_format);

        NNL_EXPECTS(!has_indices || !submesh.indexed_vertex_buffer.empty());

        submesh_off->*& RSubMesh::offset_primitives = utl::data::narrow<u32>(f.Tell());

        auto layout = vertexde::GetLayout(submesh.vertex_format);

        for (const auto& prim : submesh.primitives) {
          NNL_EXPECTS(prim.num_elements != 0);
          NNL_EXPECTS(prim.primitive_type != PrimitiveType::kTriangles || prim.num_elements % 3 == 0);

          NNL_EXPECTS(prim.primitive_type != PrimitiveType::kTriangleStrip || prim.num_elements >= 3);

          NNL_EXPECTS(prim.primitive_type != PrimitiveType::kTriangleFan || prim.num_elements >= 3);

          NNL_EXPECTS(prim.primitive_type != PrimitiveType::kLines || prim.num_elements % 2 == 0);

          NNL_EXPECTS(prim.primitive_type != PrimitiveType::kLineStrip || prim.num_elements >= 2);

          NNL_EXPECTS(prim.primitive_type != PrimitiveType::kSprites || prim.num_elements % 2 == 0);

          NNL_EXPECTS(!prim.vertex_index_buffer.empty());

          NNL_EXPECTS(!has_indices || prim.vertex_index_buffer.size() >=
                                          vertexde::GetIndexSize(submesh.vertex_format) * prim.num_elements);
          NNL_EXPECTS(has_indices || prim.vertex_index_buffer.size() >= layout.vertex_size * prim.num_elements);

#ifndef NDEBUG
          // Check if indices are valid
          if (vertexde::IsIndexed(submesh.vertex_format)) {
            for (std::size_t i = 0; i < prim.num_elements && vertexde::Is8Indices(submesh.vertex_format); i++) {
              NNL_EXPECTS(prim.vertex_index_buffer[i] < submesh.indexed_vertex_buffer.size());
            }

            for (std::size_t i = 0; i < prim.num_elements && vertexde::Is16Indices(submesh.vertex_format); i++) {
              u16 index = (prim.vertex_index_buffer[i * 2 + 1] << 8) | prim.vertex_index_buffer[i * 2];
              NNL_EXPECTS(index < submesh.indexed_vertex_buffer.size());
            }
          }
#endif
          RPrimitive rprimitive;
          rprimitive.primitive_type = utl::data::as_int(prim.primitive_type);
          rprimitive.num_elements = prim.num_elements;
          f.WriteLE(rprimitive);
        }

        f.AlignData(0x40, 0);  // display lists are aligned to 0x80

        submesh_off->*& RSubMesh::offset_display_list = utl::data::narrow<u32>(f.Tell());

        if (submesh.display_list.size() == 0) {
          for (const auto& primitive : submesh.primitives) {
            std::array<u32, 4> display_commands{
                GeCmd::kSetVertexType | submesh.vertex_format, GeCmd::kSetBaseAddress, GeCmd::kSetVramAddress,
                GeCmd::kDrawPrimitives | (utl::data::as_int(primitive.primitive_type) << 16) | primitive.num_elements};

            f.WriteLE(display_commands);
          }
        } else {
          for (const auto& display_command : submesh.display_list) f.WriteLE(display_command);
        }

        f.WriteLE(utl::data::as_int(GeCmd::kReturn));
        f.AlignData(0x10);

        if (submesh.indexed_vertex_buffer.size() > 0) {
          f.AlignData(0x40, 0);
          submesh_off->*& RSubMesh::offset_indexed_vertex_buffer = utl::data::narrow<u32>(f.Tell());
          f.WriteArrayLE(submesh.indexed_vertex_buffer);
        }

        for (const auto& primitive : submesh.primitives) {
          f.AlignData(0x40, 0);

          primitive_off->*& RPrimitive::offset_buffer = utl::data::narrow<u32>(f.Tell());

          primitive_off->*& RPrimitive::buffer_size =
              utl::data::narrow<u32>(utl::math::RoundNum(primitive.vertex_index_buffer.size(), 0x40));

          f.WriteArrayLE(primitive.vertex_index_buffer);

          f.AlignData((f.Tell() - primitive.vertex_index_buffer.size()) +
                      utl::math::RoundNum(primitive.vertex_index_buffer.size(), 0x40));

          ++primitive_off;
        }
        ++submesh_off;
      }
    }
  }
}

Buffer Export(const Model& model) {
  BufferRW f;
  Export_(model, f);
  return f;
}

void Export(const Model& model, Writer& f) { Export_(model, f); }

void Export(const Model& model, const std::filesystem::path& path) {
  FileRW f{path, true};
  Export_(model, f);
}

}  // namespace model
}  // namespace nnl
