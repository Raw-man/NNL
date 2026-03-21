#include <gtest/gtest.h>

#include "NNL/game_asset/interaction/shadow_collision.hpp"
#include "test_common.hpp"

using namespace nnl;

TEST(ShadowCollision, IsOfType) {
  FileReader bin_collision(GetPath("before_tower_shadow_collision"));

  ASSERT_TRUE(shadow_collision::IsOfType(bin_collision));
}

TEST(ShadowCollision, ImportExport) {
  {
    nnl::FileReader f{GetPath("before_tower_shadow_collision")};

    auto bin_col = f.ReadArrayLE<u8>(f.Len());

    auto collisions = shadow_collision::Import(bin_col);
    auto bin_col_re = shadow_collision::Export(collisions);

    ASSERT_TRUE(bin_col == bin_col_re);
  }
}

TEST(ShadowCollision, ConvertSModelToCollision) {
  nnl::FileReader f{GetPath("before_tower_shadow_collision")};

  auto bin_col = f.ReadArrayLE<u8>(f.Len());
  auto collision_archive = shadow_collision::Convert(shadow_collision::Convert(shadow_collision::Import(bin_col)));

  auto bin_re_convert = shadow_collision::Export(collision_archive);

  ASSERT_EQ(bin_col.size(), bin_re_convert.size());
}
