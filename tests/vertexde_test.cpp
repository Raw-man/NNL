#include "NNL/game_asset/visual/vertexde.hpp"

#include <gtest/gtest.h>

using namespace nnl;

TEST(VertexDE, VertexReader_Decode) {
  const std::size_t num_vertices = 6;

  const std::size_t expected_vert_size = 24;

  const Buffer raw_vertex_buffer(expected_vert_size * num_vertices, 0);

  const u32 format = vertexde::fmt::kUV16 | vertexde::fmt::kPosition16 | vertexde::fmt::kNormal8 |
                     vertexde::fmt::kWeight8 | vertexde::fmt::kWeightNum(8) | vertexde::fmt::kColor4444 |
                     vertexde::fmt::kIndex16;

  auto vertices = vertexde::Decode(raw_vertex_buffer, format);

  ASSERT_EQ(vertices.size(), num_vertices);
}

TEST(VertexDE, VertexReader_Encode) {
  std::vector<SVertex> vertex_array;

  for (std::size_t i = 0; i < 6; i++) {
    auto& vertex = vertex_array.emplace_back();

    vertex.position = {};
    vertex.uv = {};
    vertex.normal = {};
    vertex.bones = {0, 1, 2, 3, 4, 5, 6, 7};
    vertex.weights = {0.25f, 0.25f, 0.25f, 0.25f, 0.0f, 0.0f, 0.0f, 0.0f};
    vertex.color = {255, 255, 255, 255};
  }

  const u32 format = vertexde::fmt::kUV16 | vertexde::fmt::kPosition16 | vertexde::fmt::kNormal8 |
                     vertexde::fmt::kWeight8 | vertexde::fmt::kWeightNum(8) | vertexde::fmt::kColor4444 |
                     vertexde::fmt::kIndex16;

  ASSERT_EQ(vertexde::GetUVFormat(format), 2);
  ASSERT_EQ(vertexde::GetPositionFormat(format), 2);
  ASSERT_EQ(vertexde::GetNormalFormat(format), 1);
  ASSERT_EQ(vertexde::GetWeightFormat(format), 1);
  ASSERT_EQ(vertexde::GetIndexFormat(format), 2);
  ASSERT_EQ(vertexde::GetColorFormat(format), 6);

  ASSERT_EQ(vertexde::GetWeightNum(format), 8);
  ASSERT_EQ(vertexde::GetMorphNum(format), 1);

  auto [offset_weights, offset_uv, offset_color, offset_normal, offset_position, vertex_size] =
      vertexde::GetLayout(format);

  ASSERT_EQ(offset_weights, 0);
  ASSERT_EQ(offset_uv, 8);
  ASSERT_EQ(offset_color, 12);
  ASSERT_EQ(offset_normal, 14);
  ASSERT_EQ(offset_position, 18);
  ASSERT_EQ(vertex_size, 24);

  ASSERT_FALSE(vertexde::IsThrough(format));
}

TEST(VertexDE, vertexde_GetDescription) {
  const u32 format = vertexde::fmt::kUV16 | vertexde::fmt::kPosition16 | vertexde::fmt::kNormal8 |
                     vertexde::fmt::kWeight8 | vertexde::fmt::kWeightNum(8) | vertexde::fmt::kColor4444 |
                     vertexde::fmt::kIndex16;

  std::string description = vertexde::GetDescription(format);
  std::cout << description << std::endl;
  ASSERT_TRUE(!description.empty());
}
