
#include "NNL/game_asset/visual/fog.hpp"

#include <gtest/gtest.h>

#include "test_common.hpp"
using namespace nnl;

TEST(Fog, IsOfType) {
  FileReader bin_fog(GetPath("before_tower_fog"));

  ASSERT_TRUE(fog::IsOfType(bin_fog));
}

TEST(Fog, ImportExport) {
  {
    FileReader f{GetPath("before_tower_fog")};

    auto bin_fog = f.ReadArrayLE<u8>(f.Len());

    auto fog = fog::Import(bin_fog);
    auto bin_fog_re = fog::Export(fog);

    ASSERT_TRUE(bin_fog == bin_fog_re);
  }
}
