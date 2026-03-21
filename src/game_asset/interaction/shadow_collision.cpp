#include "NNL/game_asset/interaction/shadow_collision.hpp"

#include "NNL/utility/data.hpp"
#include "NNL/utility/string.hpp"
namespace nnl {
namespace shadow_collision {

bool operator==(const CollisionTest& lhs, const CollisionTest& rhs) {
  return lhs.triangle_indices == rhs.triangle_indices;
}

SModel Convert(const Collision& collision) {
  SModel smodel;

  std::map<u16, std::vector<STriangle>> striangles;
  for (auto& triangle : collision.triangles) {
    STriangle striangle;

    striangle[0].position = collision.vertices.at(triangle.indices_vertices[0]);
    striangle[1].position = collision.vertices.at(triangle.indices_vertices[1]);
    striangle[2].position = collision.vertices.at(triangle.indices_vertices[2]);

    striangles[triangle.shadow_features].push_back(striangle);
  }

  for (auto& [key, triangle_vec] : striangles) {
    auto& smesh = smodel.meshes.emplace_back(SMesh::FromTriangles(triangle_vec));

    smesh.name = "_shadow_" + utl::string::IntToHex(key);
  }

  SMaterial& smat = smodel.materials.emplace_back();

  smat.lit = false;

  smat.ambient = glm::vec3(0.2f);

  return smodel;
}

Collision Convert(SModel&& smodel, bool auto_cull, u32 shift) {
  Collision collision;
  NNL_EXPECTS(!smodel.meshes.empty());
  auto& smesh = smodel.meshes.at(0);

  for (std::size_t i = 1; i < smodel.meshes.size(); i++) {
    smesh.Join(smodel.meshes.at(i));
  }

  if (auto_cull) {
    std::vector<u32> new_triangles;
    new_triangles.reserve(smesh.indices.size());

    for (std::size_t i = 0; i < smesh.indices.size(); i += 3) {
      glm::vec3 p1(smesh.vertices.at(smesh.indices.at(i + 0)).position);
      glm::vec3 p2(smesh.vertices.at(smesh.indices.at(i + 1)).position);
      glm::vec3 p3(smesh.vertices.at(smesh.indices.at(i + 2)).position);

      auto U = (p2 - p1);
      auto V = (p3 - p1);

      glm::vec3 N = glm::cross(U, V);

      N = glm::normalize(N);

      auto dot = glm::dot(glm::vec3(0.0f, 1.0f, 0.0f), N);

      if (dot > 0.7f) {
        new_triangles.push_back(smesh.indices.at(i + 0));
        new_triangles.push_back(smesh.indices.at(i + 1));
        new_triangles.push_back(smesh.indices.at(i + 2));
      }
    }

    smesh.indices = std::move(new_triangles);

    smesh.RemoveDuplicateVertices();
  }

  if (smesh.indices.size() / 3 > std::numeric_limits<u16>::max())
    NNL_THROW(RuntimeError(NNL_ERMSG("the total number of triangles exceeds 65535: ")));

  collision.shift_value = shift;

  shift += 5;

  auto [min, max] = smesh.FindMinMax();

  u32 bboxX = (u32)(max.x - min.x);

  u32 bboxY = (u32)(max.y - min.y);

  u32 bboxZ = (u32)(max.z - min.z);

  bboxX = (bboxX >> shift) + 1;

  bboxY = (bboxY >> shift) + 1;

  bboxZ = (bboxZ >> shift) + 1;

  collision.coordinate_map.Resize(bboxZ, bboxY, bboxX);

  collision.origin_point = min;

  collision.triangles.reserve(smesh.indices.size() / 3);

  std::vector<std::pair<glm::vec3, glm::vec3>> triangle_min_max(smesh.indices.size() / 3);

  for (std::size_t i = 0; i < smesh.indices.size(); i += 3) {
    auto& triangle = collision.triangles.emplace_back();

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

    triangle_min_max[i / 3] = std::make_pair(min_vec, max_vec);
  }

  auto calc_distance = [min = min, shift = shift](glm::vec3 p) {
    auto d = p - min;
    return Vec3<u32>{(u32)d.x >> shift, (u32)d.y >> shift, (u32)d.z >> shift};
  };

  for (std::size_t i = 0; i < collision.triangles.size(); i++) {
    auto min_p = triangle_min_max[i].first;
    auto max_p = triangle_min_max[i].second;

    auto min_d = calc_distance(min_p);
    auto max_d = calc_distance(max_p);
    for (std::size_t z = min_d.z; z <= max_d.z; z++) {
      for (std::size_t y = min_d.y; y <= max_d.y; y++) {
        for (std::size_t x = min_d.x; x <= max_d.x; x++) {
          auto& collision_test = collision.coordinate_map(z, y, x);
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

bool IsOfType_(Reader& f) {
  using namespace raw;

  f.Seek(0);

  const std::size_t data_size = f.Len();

  if (data_size < sizeof(RHeader)) return false;

  auto rheader = f.ReadLE<RHeader>();

  if (rheader.offset_vertex_table != sizeof(RHeader)) return false;

  if (rheader.bbox_dimension.x == 0 || rheader.bbox_dimension.y == 0 || rheader.bbox_dimension.z == 0) return false;

  if (rheader.origin_point.w > 1.0f) return false;

  if (rheader.offset_vertex_table < sizeof(RHeader) || data_size < rheader.offset_vertex_table ||
      rheader.offset_vertex_table % sizeof(Vec4<float>) != 0)
    return false;

  if (rheader.offset_coordinate_map < sizeof(RHeader) ||
      data_size < rheader.offset_coordinate_map +
                      sizeof(u16) * rheader.bbox_dimension.z * rheader.bbox_dimension.y * rheader.bbox_dimension.x)
    return false;

  if (rheader.offset_triangles < sizeof(RHeader) || data_size < rheader.offset_triangles) return false;

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

    u32 num_vertices = (collision.header.offset_triangles - collision.header.offset_vertex_table) / sizeof(Vec4<float>);

    collision.vertices = f.ReadArrayLE<Vec4<float>>(num_vertices);

    f.Seek(collision.header.offset_triangles);

    u32 num_triangles =
        (collision.header.offset_collision_tests - collision.header.offset_triangles) / sizeof(RTriangle);

    collision.triangles = f.ReadArrayLE<RTriangle>(num_triangles);

    f.Seek(collision.header.offset_collision_tests);

    auto first_entry = f.ReadLE<RCollisionTest>();

    f.Seek(collision.header.offset_collision_tests);

    u32 num_collision_tests =
        (first_entry.offset_triangle_indices - collision.header.offset_collision_tests) / sizeof(RCollisionTest);

    collision.collision_tests = f.ReadArrayLE<RCollisionTest>(num_collision_tests);

    f.Seek(first_entry.offset_triangle_indices);

    u32 num_indices = (header.offset_coordinate_map - first_entry.offset_triangle_indices) / sizeof(u16);

    collision.indices_triangles = f.ReadArrayLE<u16>(num_indices);
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
            collision.coordinate_map.At(z, y, x).triangle_indices.push_back(rcollision.indices_triangles.at(i));
          }
        }
      }
    }
  }

  for (auto& rtriangle : rcollision.triangles) {
    Triangle triangle;
    triangle.shadow_features = rtriangle.shadow_features;
    triangle.indices_vertices[0] = rtriangle.index_vertex_0;
    triangle.indices_vertices[1] = rtriangle.index_vertex_1;
    triangle.indices_vertices[2] = rtriangle.index_vertex_2;
    collision.triangles.push_back(triangle);
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

  auto header_off = f.WriteLE(header);

  for (auto& vertex : collision.vertices) f.WriteLE(Vec4<float>{vertex.x, vertex.y, vertex.z, 1.0f});

  header_off->*& RHeader::offset_triangles = utl::data::narrow<u32>(f.Tell());

  for (auto& triangle : collision.triangles) {
    RTriangle rtriangle;

    for (std::size_t i = 0; i < 3; i++) {
      NNL_EXPECTS(triangle.indices_vertices[i] < collision.vertices.size());
    }

    rtriangle.index_vertex_0 = triangle.indices_vertices[0];
    rtriangle.index_vertex_1 = triangle.indices_vertices[1];
    rtriangle.index_vertex_2 = triangle.indices_vertices[2];
    rtriangle.shadow_features = triangle.shadow_features;
    f.WriteLE(rtriangle);
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
    rcollision_test.num_triangle_indices = collision_test.triangle_indices.size();
    f.WriteLE(rcollision_test);
  }

  for (auto& collision_test : collision_tests) {
    collision_test_start->*& RCollisionTest::offset_triangle_indices = utl::data::narrow<u32>(f.Tell());

#ifndef NDEBUG
    for (auto index : collision_test.triangle_indices) {
      assert(index < collision.triangles.size());
    }
#endif
    f.WriteArrayLE(collision_test.triangle_indices);

    ++collision_test_start;
  }

  header_off->*& RHeader::offset_coordinate_map = utl::data::narrow<u32>(f.Tell());

  f.WriteArrayLE(map);
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

}  // namespace shadow_collision
}  // namespace nnl
