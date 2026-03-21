#include "NNL/game_asset/interaction/colbox.hpp"

#include <gtest/gtest.h>

#include "test_common.hpp"
using namespace nnl;

TEST(Colbox, IsOfType) {
  FileReader bin_hitbox(GetPath("naruto_colbox"));

  ASSERT_TRUE(colbox::IsOfType(bin_hitbox));
}

TEST(Colbox, ImportExport) {
  {
    nnl::FileReader f{GetPath("naruto_colbox")};

    auto bin_hitbox = f.ReadArrayLE<u8>(f.Len());

    auto hitbox = colbox::Import(bin_hitbox);
    auto bin_hitbox_re = colbox::Export(hitbox);

    ASSERT_TRUE(bin_hitbox == bin_hitbox_re);
  }
}
