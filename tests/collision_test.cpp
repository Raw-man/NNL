#include "NNL/game_asset/interaction/collision.hpp"

#include <gtest/gtest.h>

#include "test_common.hpp"

using namespace nnl;

TEST(Collision, IsOfType) {
  FileReader bin_collision(GetPath("before_tower_collision"));
  ASSERT_TRUE(collision::IsOfType(bin_collision));
}

TEST(Collision, ImportExport) {
  {
    nnl::FileReader f{GetPath("before_tower_collision")};

    auto bin_col = f.ReadArrayLE<u8>(f.Len());

    auto collisions = collision::Import(bin_col);
    auto bin_col_re = collision::Export(collisions);

    ASSERT_TRUE(bin_col == bin_col_re);
  }
}

TEST(Collision, ConvertCollisionToSModel) {
  nnl::FileReader f{GetPath("before_tower_collision")};

  auto bin_col = f.ReadArrayLE<u8>(f.Len());

  auto collision_archive = collision::Import(bin_col);

  auto smodel = collision::Convert(collision_archive);

  ASSERT_EQ(smodel.meshes.size(), 3);
}

TEST(Collision, ConvertSModelToCollision) {
  nnl::FileReader f{GetPath("before_tower_collision")};

  auto bin_col = f.ReadArrayLE<u8>(f.Len());

  auto collision_archive = collision::Import(bin_col);

  auto smodel = collision::Convert(collision_archive);

  auto collision_archive_re = collision::Convert(std::move(smodel));

  ASSERT_EQ(collision_archive.triangles.size(), collision_archive_re.triangles.size());
}
