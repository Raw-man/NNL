#include "NNL/game_asset/visual/model.hpp"

#include <gtest/gtest.h>

#include "NNL/common/fixed_type.hpp"
#include "test_common.hpp"

using namespace nnl;

TEST(Model, IsOfType) {
  FileReader bin_asset(GetPath("naruto_model"));

  ASSERT_TRUE(model::IsOfType(bin_asset));
}

TEST(Model, ExportDefaultModel) {
  model::Model mdl;
  mdl.skeleton = {{}};
  mdl.mesh_groups = {{}};
  auto bin_model = model::Export(mdl);
  ASSERT_TRUE(bin_model.size() >= sizeof(model::raw::RHeader));
}

TEST(Model, ImportReExport) {
  {
    nnl::FileReader f{GetPath("naruto_model")};

    auto bin_asset = f.ReadArrayLE<u8>(f.Len());

    auto model = model::Import(bin_asset);

    auto bin_asset_re = model::Export(model);

    ASSERT_TRUE(bin_asset_re == bin_asset);
  }

  {
    nnl::FileReader f{GetPath("before_tower_model")};

    auto bin_asset = f.ReadArrayLE<u8>(f.Len());

    auto model = model::Import(bin_asset);

    auto bin_asset_re = model::Export(model);

    ASSERT_TRUE(bin_asset_re == bin_asset);
  }
}

TEST(Model, GenerateBoneRoleTable) {
  {
    model::Model model = model::Import(GetPath("naruto_model"));

    auto smodel = model::Convert(model);

    auto new_table = model::GenerateBoneTargetTables(smodel.skeleton);

    ASSERT_TRUE(new_table == model.bone_target_tables);
  }
}

TEST(Model, ConvertModelToSModel) {
  nnl::FileReader f{GetPath("naruto_model")};

  auto bin_asset = f.ReadArrayLE<u8>(f.Len());

  auto model = model::Import(bin_asset);
  auto smodel = model::Convert(model);

  auto original_mesh_count = 0;

  for (const auto& mesh_group : model.mesh_groups) {
    original_mesh_count += mesh_group.meshes.size();
  }

  ASSERT_EQ(smodel.meshes.size(), original_mesh_count);
}

TEST(Model, GroupTrianglesByBones) {
  std::vector<u32> indices;
  std::vector<SVertex> vertices;

  for (std::size_t i = 0; i < 86 * 4; i++) {
    indices.insert(indices.end(), {(u32)i, (u32)i, (u32)i});

    auto& vertex = vertices.emplace_back();

    vertex.weights = {{0, 0.25f}, {0, 0.25f}, {0, 0.25f}};
  }

  auto groups = model::GroupTrianglesByBones(indices, vertices, 86);

  ASSERT_EQ(groups.size(), 4);

  for (std::size_t i = 0; i < vertices.size(); i++) {
    vertices.at(i).weights = {{(u16)i, 0.25f}, {(u16)(i + 1), 0.25f}, {(u16)(i + 2), 0.25f}};
  }

  groups = model::GroupTrianglesByBones(indices, vertices, -1, true);

  ASSERT_EQ(groups.size(), 86);
}

TEST(Model, ConvertBonesToSSkeleton) {
  std::vector<model::Bone> bones{
      {-1, glm::vec3{0.0f}, glm::vec3{0.0f}, glm::vec3{0.0f}}, {0, glm::vec3{0.0f}, glm::vec3{0.0f}, glm::vec3{0.0f}},
      {1, glm::vec3{0.0f}, glm::vec3{0.0f}, glm::vec3{0.0f}},  {1, glm::vec3{0.0f}, glm::vec3{0.0f}, glm::vec3{0.0f}},
      {1, glm::vec3{0.0f}, glm::vec3{0.0f}, glm::vec3{0.0f}},  {-1, glm::vec3{0.0f}, glm::vec3{0.0f}, glm::vec3{0.0f}},
      {5, glm::vec3{0.0f}, glm::vec3{0.0f}, glm::vec3{0.0f}},  {6, glm::vec3{0.0f}, glm::vec3{0.0f}, glm::vec3{0.0f}},
      {6, glm::vec3{0.0f}, glm::vec3{0.0f}, glm::vec3{0.0f}}};

  auto sskeleton = model::Convert(bones);
  ASSERT_EQ(sskeleton.roots.size(), 2);
  ASSERT_EQ(sskeleton.roots[0].children.size(), 1);
  ASSERT_EQ(sskeleton.roots[1].children.size(), 1);
  ASSERT_EQ(sskeleton.roots[0].children[0].children.size(), 3);
  ASSERT_EQ(sskeleton.roots[1].children[0].children.size(), 2);
}
