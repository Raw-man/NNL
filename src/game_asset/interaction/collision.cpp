#include "NNL/game_asset/interaction/collision.hpp"

#include "NNL/common/contract.hpp"
#include "NNL/utility/data.hpp"
#include "NNL/utility/string.hpp"
namespace nnl {
namespace collision {

bool operator==(const CollisionTest& lhs, const CollisionTest& rhs) {
  return lhs.triangle_indices == rhs.triangle_indices && lhs.triangle_wall_indices == rhs.triangle_wall_indices &&
         lhs.edge_wall_indices == rhs.edge_wall_indices;
}

SModel Convert(const Collision& collision) {
  SModel smodel;

  std::map<u32, std::vector<STriangle>> striangles;
  for (auto& triangle : collision.triangles) {
    STriangle striangle;

    striangle[0].position = collision.vertices.at(triangle.indices_vertices[2]);
    striangle[0].normal = -1.0f * collision.normals.at(triangle.index_normal);
    striangle[1].position = collision.vertices.at(triangle.indices_vertices[1]);
    striangle[1].normal = -1.0f * collision.normals.at(triangle.index_normal);
    striangle[2].position = collision.vertices.at(triangle.indices_vertices[0]);
    striangle[2].normal = -1.0f * collision.normals.at(triangle.index_normal);
    striangles[(utl::data::as_int(triangle.push_features) << 16) | utl::data::as_int(triangle.surface_features)]
        .push_back(striangle);
  }
  for (auto& [key, triangle_vec] : striangles) {
    SMesh& smesh = smodel.meshes.emplace_back(SMesh::FromTriangles(triangle_vec));

    SurfaceFeatures surface = static_cast<SurfaceFeatures>(key & 0xFFFF);

    PushFeatures push = static_cast<PushFeatures>(key >> 16);

    smesh.name += "_surf_" + utl::string::IntToHex(utl::data::as_int(surface));
    smesh.name += "_push_" + utl::string::IntToHex(utl::data::as_int(push));

    smesh.material_id = smodel.materials.size();

    SMaterial& smat = smodel.materials.emplace_back();

    smat.name = smesh.name + "_mat_" + std::to_string(smodel.materials.size() - 1);

    smat.lit = false;

    glm::vec3 surface_color;

    SurfaceFeatures surface_mask{0xF};

    switch (surface & surface_mask) {
      case SurfaceFeatures::kNone:
      case SurfaceFeatures::kDirt:
        surface_color = glm::vec3(0.76f, 0.72f, 0.50f);
        break;

      case SurfaceFeatures::kDirt2:
        surface_color = glm::vec3(0.60f, 0.57f, 0.40f);
        break;

      case SurfaceFeatures::kSand:
        surface_color = glm::vec3(0.96f, 0.71f, 0.20f);
        break;

      case SurfaceFeatures::kStone:
        surface_color = glm::vec3(0.60f, 0.60f, 0.62f);
        break;

      case SurfaceFeatures::kStone2:
        surface_color = glm::vec3(0.67f, 0.67f, 0.67f);
        break;

      case SurfaceFeatures::kGrass:
        surface_color = glm::vec3(0.57f, 0.86f, 0.05f);
        break;

      case SurfaceFeatures::kWater:
        surface_color = glm::vec3(0.03f, 0.46f, 0.57f);
        break;

      case SurfaceFeatures::kWood:
        surface_color = glm::vec3(0.78f, 0.64f, 0.50f);
        break;

      case SurfaceFeatures::kMetal:
        surface_color = glm::vec3(0.15f, 0.15f, 0.15f);
        break;

      default:
        assert(false);
    }

    smat.ambient = surface_color;

    SurfaceFeatures pass_mask{0x600};

    if ((surface & pass_mask) != SurfaceFeatures::kNone) {
      smat.opacity = 0.5f;
      smat.alpha_mode = SBlendMode::kAlpha;
    }

    if ((surface & SurfaceFeatures::kShadow) != SurfaceFeatures::kNone) {
      smat.ambient *= 0.8;
    }

    if (push != PushFeatures::kNone) smat.ambient = glm::vec3(1.0f, 0.0f, 0.0f);
  }

  return smodel;
}

std::vector<ConvertParam> GenerateConvertParam(const Collision& collision) {
  std::set<u32> surface_groups;

  for (auto& triangle : collision.triangles) {
    surface_groups.insert((utl::data::as_int(triangle.push_features) << 16) |
                          utl::data::as_int(triangle.surface_features));
  }

  std::vector<ConvertParam> params;
  params.reserve(surface_groups.size());

  for (auto& surface_group : surface_groups) {
    ConvertParam param;
    param.push_features = static_cast<PushFeatures>(surface_group >> 16);
    param.surface_features = static_cast<SurfaceFeatures>(surface_group & 0xFFFF);
    params.push_back(param);
  }

  return params;
}

Collision Convert_(SModel&& smodel, const std::vector<ConvertParam>& mesh_params, bool auto_wall, u32 shift) {
  NNL_EXPECTS(!smodel.meshes.empty());
  NNL_EXPECTS(mesh_params.size() == smodel.meshes.size());

  Collision collision;

  auto& smesh = smodel.meshes.at(0);

  // A temporary struct to store params for individual triangles
  std::vector<ConvertParam> param_map(smesh.indices.size() / 3, mesh_params.at(0));

  for (std::size_t i = 1; i < smodel.meshes.size(); i++) {
    smesh.Join(smodel.meshes.at(i));
    param_map.resize(smesh.indices.size() / 3, mesh_params.at(i));
  }

  if (smesh.indices.size() / 3 > std::numeric_limits<u16>::max())
    NNL_THROW(RuntimeError(
        NNL_ERMSG("the total number of triangles exceeds 65535: " + std::to_string(smesh.indices.size() / 3))));

  smesh.ReverseWindingOrder();

  auto [min, max] = smesh.FindMinMax();

  u32 bboxX = (u32)(max.x - min.x);

  u32 bboxY = (u32)(max.y - min.y);

  u32 bboxZ = (u32)(max.z - min.z);

  bboxX = (bboxX >> shift) + 1;

  bboxY = (bboxY >> shift) + 1;

  bboxZ = (bboxZ >> shift) + 1;

  collision.coordinate_map.Resize(bboxZ, bboxY, bboxX);

  collision.shift_value = shift;
  collision.origin_point = min;

  for (std::size_t i = 0; i < smesh.indices.size(); i += 3) {
    auto& triangle = collision.triangles.emplace_back();

    triangle.surface_features = param_map.at(i / 3).surface_features;

    auto find_index = [](auto& element, auto& container) -> u16 {
      for (std::size_t k = 0; k < container.size(); k++) {
        if (element == container[k]) {
          return k;
        }
      }

      container.push_back(element);
      return container.size() - 1;
    };

    for (std::size_t j = 0; j < 3; j++) {
      auto& vertex = smesh.vertices.at(smesh.indices.at(i + j));
      triangle.indices_vertices[j] = find_index(vertex.position, collision.vertices);
    }

    // select min max points
    auto vertex_0 = collision.vertices.at(triangle.indices_vertices[0]);
    auto vertex_1 = collision.vertices.at(triangle.indices_vertices[1]);
    auto vertex_2 = collision.vertices.at(triangle.indices_vertices[2]);

    auto min_vec =
        glm::vec3(std::min({vertex_0.x, vertex_1.x, vertex_2.x}), std::min({vertex_0.y, vertex_1.y, vertex_2.y}),
                  std::min({vertex_0.z, vertex_1.z, vertex_2.z}));

    auto max_vec =
        glm::vec3(std::max({vertex_0.x, vertex_1.x, vertex_2.x}), std::max({vertex_0.y, vertex_1.y, vertex_2.y}),
                  std::max({vertex_0.z, vertex_1.z, vertex_2.z}));

    triangle.index_vertex_min = find_index(min_vec, collision.vertices);
    triangle.index_vertex_max = find_index(max_vec, collision.vertices);

    glm::vec3 p1(smesh.vertices.at(smesh.indices.at(i + 0)).position);
    glm::vec3 p2(smesh.vertices.at(smesh.indices.at(i + 1)).position);
    glm::vec3 p3(smesh.vertices.at(smesh.indices.at(i + 2)).position);

    auto U = (p2 - p1);
    auto V = (p3 - p1);

    glm::vec3 N = glm::cross(U, V);

    N = glm::normalize(N);

    auto dot = glm::dot(glm::vec3(0, -1.0, 0), N);

    triangle.push_features = param_map.at(i / 3).push_features;

    if (auto_wall && dot <= 0.7f && dot >= -0.7f)  // Wall
      triangle.push_features |= PushFeatures::kPushBack;

    if (auto_wall && dot < -0.7f) triangle.push_features |= PushFeatures::kPushIfStuck;

    if (auto_wall && ((triangle.push_features & PushFeatures::kPushBack) != PushFeatures::kNone) &&
        ((triangle.surface_features & SurfaceFeatures::kPassThrowables) != SurfaceFeatures::kNone)) {
      triangle.push_features |= PushFeatures::kPushIfStuck;
    }

    triangle.index_normal = find_index(N, collision.normals);
  }

  auto calc_distance = [min = min, shift = shift](glm::vec3 p) {
    auto d = p - min;
    return Vec3<u32>{(u32)d.x >> shift, (u32)d.y >> shift, (u32)d.z >> shift};
  };

  for (std::size_t i = 0; i < collision.triangles.size(); i++) {
    auto& triangle = collision.triangles[i];
    auto min_p = collision.vertices[triangle.index_vertex_min];
    auto max_p = collision.vertices[triangle.index_vertex_max];

    auto min_d = calc_distance(min_p);
    auto max_d = calc_distance(max_p);
    for (std::size_t z = min_d.z; z <= max_d.z; z++) {
      for (std::size_t y = min_d.y; y <= max_d.y; y++) {
        for (std::size_t x = min_d.x; x <= max_d.x; x++) {
          auto& collision_test = collision.coordinate_map(z, y, x);

          if ((triangle.push_features & PushFeatures::kPushBack) != PushFeatures::kNone)
            collision_test.triangle_wall_indices.push_back(i);
          else
            collision_test.triangle_indices.push_back(i);
        }
      }
    }
  }

  if (collision.vertices.size() > std::numeric_limits<u16>::max())
    NNL_THROW(RuntimeError(
        NNL_ERMSG("the total number of vertices exceeds 65535: " + std::to_string(collision.vertices.size()))));
  smodel = {};
  return collision;
}

Collision Convert(SModel&& smodel, const std::vector<ConvertParam>& mesh_params, bool auto_wall, u32 shift) {
  return Convert_(std::move(smodel), mesh_params, auto_wall, shift);
}

Collision Convert(SModel&& smodel, const ConvertParam& mesh_params, bool auto_wall, u32 shift) {
  std::vector<ConvertParam> params(smodel.meshes.size(), mesh_params);
  return Convert_(std::move(smodel), params, auto_wall, shift);
}

bool IsOfType_(Reader& f) {
  using namespace raw;

  f.Seek(0);

  const std::size_t data_size = f.Len();

  if (data_size < sizeof(RHeader)) return false;

  auto rheader = f.ReadLE<RHeader>();

  if (rheader.bbox_dimension.x == 0 || rheader.bbox_dimension.y == 0 || rheader.bbox_dimension.z == 0) return false;

  if (rheader.origin_point.w > 1.0f) return false;

  if (rheader.offset_vertex_table < sizeof(RHeader) || data_size < rheader.offset_vertex_table ||
      rheader.offset_vertex_table % sizeof(Vec4<float>) != 0)
    return false;

  if (rheader.offset_normal_table < sizeof(RHeader) || data_size < rheader.offset_normal_table ||
      rheader.offset_normal_table % sizeof(Vec4<float>) != 0)
    return false;

  if (rheader.offset_coordinate_map < sizeof(RHeader) ||
      data_size < rheader.offset_coordinate_map +
                      sizeof(u16) * rheader.bbox_dimension.z * rheader.bbox_dimension.y * rheader.bbox_dimension.x)
    return false;

  if (rheader.offset_edges < sizeof(RHeader) || data_size < rheader.offset_edges) return false;

  if (rheader.offset_triangles < sizeof(RHeader) ||
      data_size < rheader.offset_triangles + rheader.num_triangles * sizeof(RTriangle))
    return false;

  return true;
}

bool IsOfType(Reader& f) { return IsOfType_(f); }

bool IsOfType(BufferView buffer) { return IsOfType_(buffer); }

bool IsOfType(const std::filesystem::path& path) {
  FileReader f{path};
  return IsOfType_(f);
}

namespace raw {

RCollision Parse(Reader& f) {
  f.Seek(0);
  RCollision collision;
  NNL_TRY {
    collision.header = f.ReadLE<RHeader>();
    auto& header = collision.header;

    if (header.offset_vertex_table != sizeof(RHeader)) NNL_THROW(ParseError(NNL_ERMSG("invalid header size")));

    f.Seek(header.offset_coordinate_map);
    u32 num_coordinates = header.bbox_dimension.x * header.bbox_dimension.y * header.bbox_dimension.z;
    collision.coordinate_map = f.ReadArrayLE<u16>(num_coordinates);

    f.Seek(collision.header.offset_vertex_table);

    u32 num_vertices =
        (collision.header.offset_normal_table - collision.header.offset_vertex_table) / sizeof(Vec4<float>);

    collision.vertices = f.ReadArrayLE<Vec4<float>>(num_vertices);

    f.Seek(collision.header.offset_normal_table);

    u32 num_normals = (collision.header.offset_triangles - collision.header.offset_normal_table) / sizeof(Vec4<float>);

    collision.normals = f.ReadArrayLE<Vec4<float>>(num_normals);

    f.Seek(collision.header.offset_triangles);

    u32 num_triangles = collision.header.num_triangles;

    collision.triangles = f.ReadArrayLE<RTriangle>(num_triangles);

    f.Seek(collision.header.offset_edges);

    u32 num_lines = (collision.header.offset_collision_tests - collision.header.offset_edges) / sizeof(REdge);

    collision.edges = f.ReadArrayLE<REdge>(num_lines);

    f.Seek(collision.header.offset_collision_tests);

    auto first_entry = f.ReadLE<RCollisionTest>();

    f.Seek(collision.header.offset_collision_tests);

    u32 num_collision_tests =
        (first_entry.offset_triangle_indices - collision.header.offset_collision_tests) / sizeof(RCollisionTest);

    collision.collision_tests = f.ReadArrayLE<RCollisionTest>(num_collision_tests);

    f.Seek(first_entry.offset_triangle_indices);

    u32 num_indices = (header.offset_coordinate_map - first_entry.offset_triangle_indices) / sizeof(u16);

    collision.indices_triangles_edges = f.ReadArrayLE<u16>(num_indices);
  }
  NNL_CATCH(std::exception) { NNL_THROW(ParseError{NNL_ERMSG(e.what())}); }
  return collision;
}

Collision Convert(const RCollision& rcollision) {
  Collision collision;

  collision.shift_value = rcollision.header.shift_value;
  collision.origin_point =
      glm::vec3(rcollision.header.origin_point.x, rcollision.header.origin_point.y, rcollision.header.origin_point.z);
  collision.coordinate_map.Resize(rcollision.header.bbox_dimension.z, rcollision.header.bbox_dimension.y,
                                  rcollision.header.bbox_dimension.x);
  auto first_off = rcollision.collision_tests.at(0).offset_triangle_indices;

  for (std::size_t z = 0; z < rcollision.header.bbox_dimension.z; z++) {
    for (std::size_t y = 0; y < rcollision.header.bbox_dimension.y; y++) {
      for (std::size_t x = 0; x < rcollision.header.bbox_dimension.x; x++) {
        auto rcollision_test_index =
            rcollision.coordinate_map.at(x + y * rcollision.header.bbox_dimension.x +
                                         rcollision.header.bbox_dimension.x * rcollision.header.bbox_dimension.y * z);
        auto& test = rcollision.collision_tests.at(rcollision_test_index);

        if (test.num_triangle_indices > 0) {
          auto index_face = (test.offset_triangle_indices - first_off) / sizeof(u16);
          for (std::size_t i = index_face; i < index_face + test.num_triangle_indices; i++) {
            collision.coordinate_map.At(z, y, x).triangle_indices.push_back(rcollision.indices_triangles_edges.at(i));
          }
        }

        if (test.num_triangle_wall_indices > 0) {
          auto index_face = (test.offset_triangle_wall_indices - first_off) / sizeof(u16);
          for (std::size_t i = index_face; i < index_face + test.num_triangle_wall_indices; i++) {
            collision.coordinate_map.At(z, y, x).triangle_wall_indices.push_back(
                rcollision.indices_triangles_edges.at(i));
          }
        }

        if (test.num_edge_indices > 0) {
          auto index_face = (test.offset_edge_indices - first_off) / sizeof(u16);
          for (std::size_t i = index_face; i < index_face + test.num_edge_indices; i++) {
            collision.coordinate_map.At(z, y, x).edge_wall_indices.push_back(rcollision.indices_triangles_edges.at(i));
          }
        }
      }
    }
  }

  for (auto& rtriangle : rcollision.triangles) {
    Triangle triangle;
    triangle.index_normal = rtriangle.index_normal;
    triangle.indices_vertices[0] = rtriangle.index_vertex_0;
    triangle.indices_vertices[1] = rtriangle.index_vertex_1;
    triangle.indices_vertices[2] = rtriangle.index_vertex_2;
    triangle.index_vertex_min = rtriangle.index_vertex_min;
    triangle.index_vertex_max = rtriangle.index_vertex_max;
    triangle.push_features = static_cast<PushFeatures>(rtriangle.push_features);
    triangle.surface_features = static_cast<SurfaceFeatures>(rtriangle.surface_features);
    collision.triangles.push_back(triangle);
  }

  for (auto& rline : rcollision.edges) {
    Edge line;
    line.index_normal = rline.index_normal;
    line.indices_vertices[0] = rline.index_vertex_0;
    line.indices_vertices[1] = rline.index_vertex_1;
    line.index_vertex_min = rline.index_vertex_min;
    line.index_vertex_max = rline.index_vertex_max;
    line.surface_features = static_cast<SurfaceFeatures>(rline.mask);
    collision.edges.push_back(line);
  }

  for (auto& rnormal : rcollision.normals) {
    collision.normals.push_back(glm::vec3(rnormal.x, rnormal.y, rnormal.z));
  }

  for (auto& rvertex : rcollision.vertices) {
    collision.vertices.push_back(glm::vec3(rvertex.x, rvertex.y, rvertex.z));
  }

  return collision;
}

}  // namespace raw

Collision Import_(Reader& f) { return raw::Convert(raw::Parse(f)); }

Collision Import(Reader& f) { return Import_(f); }

Collision Import(BufferView buffer) { return Import_(buffer); }

Collision Import(const std::filesystem::path& path) {
  FileReader f{path};

  return Import_(f);
}

void Export_(const Collision& collision, Writer& f) {
  using namespace raw;

  f.Seek(0);

  RHeader header;
  header.origin_point = {collision.origin_point.x, collision.origin_point.y, collision.origin_point.z, 1.0f};

  header.bbox_dimension.x = utl::data::narrow<u32>(collision.coordinate_map.SizeX());
  header.bbox_dimension.y = utl::data::narrow<u32>(collision.coordinate_map.SizeY());
  header.bbox_dimension.z = utl::data::narrow<u32>(collision.coordinate_map.SizeZ());

  header.shift_value = collision.shift_value;

  header.offset_vertex_table = sizeof(RHeader);

  header.num_triangles = utl::data::narrow<u16>(collision.triangles.size());

  auto header_off = f.WriteLE(header);

  for (auto& vertex : collision.vertices) f.WriteLE(Vec4<float>{vertex.x, vertex.y, vertex.z, 1.0f});

  header_off->*& RHeader::offset_normal_table = utl::data::narrow<u32>(f.Tell());

  for (auto& normal : collision.normals) f.WriteLE(Vec4<float>{normal.x, normal.y, normal.z, 1.0f});

  header_off->*& RHeader::offset_triangles = utl::data::narrow<u32>(f.Tell());

  for (auto& triangle : collision.triangles) {
    RTriangle rtriangle;

    NNL_EXPECTS(triangle.index_normal < collision.normals.size());

    for (std::size_t i = 0; i < 3; i++) NNL_EXPECTS(triangle.indices_vertices[i] < collision.vertices.size());

    NNL_EXPECTS(triangle.index_vertex_min < collision.vertices.size());
    NNL_EXPECTS(triangle.index_vertex_max < collision.vertices.size());

    rtriangle.index_normal = triangle.index_normal;
    rtriangle.index_vertex_0 = triangle.indices_vertices[0];
    rtriangle.index_vertex_1 = triangle.indices_vertices[1];
    rtriangle.index_vertex_2 = triangle.indices_vertices[2];
    rtriangle.index_vertex_min = triangle.index_vertex_min;
    rtriangle.index_vertex_max = triangle.index_vertex_max;
    rtriangle.surface_features = utl::data::as_int(triangle.surface_features);
    rtriangle.push_features = utl::data::as_int(triangle.push_features);
    f.WriteLE(rtriangle);
  }

  header_off->*& RHeader::offset_edges = utl::data::narrow<u32>(f.Tell());

  for (auto& line : collision.edges) {
    REdge rline;

    NNL_EXPECTS(line.index_normal < collision.normals.size());

    NNL_EXPECTS(line.indices_vertices[0] < collision.vertices.size());
    NNL_EXPECTS(line.indices_vertices[1] < collision.vertices.size());

    NNL_EXPECTS(line.index_vertex_min < collision.vertices.size());
    NNL_EXPECTS(line.index_vertex_max < collision.vertices.size());

    rline.index_normal = line.index_normal;
    rline.index_vertex_0 = line.indices_vertices[0];
    rline.index_vertex_1 = line.indices_vertices[1];
    rline.index_vertex_min = line.index_vertex_min;
    rline.index_vertex_max = line.index_vertex_max;
    rline.mask = utl::data::as_int(line.surface_features);
    f.WriteLE(rline);
  }

  std::vector<u16> map;
  map.reserve(header.bbox_dimension.z * header.bbox_dimension.y * header.bbox_dimension.x);
  std::vector<CollisionTest> collision_tests;
  for (std::size_t z = 0; z < header.bbox_dimension.z; z++) {
    for (std::size_t y = 0; y < header.bbox_dimension.y; y++) {
      for (std::size_t x = 0; x < header.bbox_dimension.x; x++) {
        auto& test = collision.coordinate_map.At(z, y, x);
        auto itr = std::find_if(collision_tests.begin(), collision_tests.end(),
                                [&test](auto& rhs) -> bool { return rhs == test; });
        if (itr != collision_tests.end()) {
          u16 index = itr - collision_tests.begin();
          map.push_back(index);
        } else {
          map.push_back(utl::data::narrow_cast<u16>(collision_tests.size()));
          collision_tests.push_back(test);
        }
      }
    }
  }
  header_off->*& RHeader::offset_collision_tests = utl::data::narrow<u32>(f.Tell());

  auto collision_test_start = f.MakeOffsetLE<RCollisionTest>();

  for (auto& collision_test : collision_tests) {
    RCollisionTest rcollision_test;
    rcollision_test.num_triangle_indices = utl::data::narrow<u16>(collision_test.triangle_indices.size());
    rcollision_test.num_triangle_wall_indices = utl::data::narrow<u16>(collision_test.triangle_wall_indices.size());
    rcollision_test.num_edge_indices = utl::data::narrow<u16>(collision_test.edge_wall_indices.size());
    f.WriteLE(rcollision_test);
  }

  for (auto& collision_test : collision_tests) {
    collision_test_start->*& RCollisionTest::offset_triangle_indices = utl::data::narrow<u32>(f.Tell());

#ifndef NDEBUG
    for (auto index : collision_test.triangle_indices) {
      NNL_EXPECTS_DBG(index < collision.triangles.size());
    }

    for (auto index : collision_test.triangle_wall_indices) {
      NNL_EXPECTS_DBG(index < collision.triangles.size());
    }

    for (auto index : collision_test.edge_wall_indices) {
      NNL_EXPECTS_DBG(index < collision.edges.size());
    }
#endif

    f.WriteArrayLE(collision_test.triangle_indices);

    collision_test_start->*& RCollisionTest::offset_triangle_wall_indices = utl::data::narrow<u32>(f.Tell());

    f.WriteArrayLE(collision_test.triangle_wall_indices);

    collision_test_start->*& RCollisionTest::offset_edge_indices = utl::data::narrow<u32>(f.Tell());

    f.WriteArrayLE(collision_test.edge_wall_indices);

    ++collision_test_start;
  }

  header_off->*& RHeader::offset_coordinate_map = utl::data::narrow<u32>(f.Tell());

  f.WriteArrayLE(map);

  header_off->*& RHeader::file_size = utl::data::narrow<u32>(f.Tell());
}

Buffer Export(const Collision& collision) {
  BufferRW f;
  Export_(collision, f);
  return f;
}

void Export(const Collision& collision, Writer& f) { Export_(collision, f); }

void Export(const Collision& collision, const std::filesystem::path& path) {
  FileRW f{path, true};
  Export_(collision, f);
}

}  // namespace collision

}  // namespace nnl
