#include <gtest/gtest.h>

#include "NNL/game_asset/visual/visanimation.hpp"
#include "test_common.hpp"

using namespace nnl;

TEST(Visanimation, IsOfType) {
  FileReader bin_subanimation(GetPath("naruto_visanimation"));

  ASSERT_TRUE(visanimation::IsOfType(bin_subanimation));
}

TEST(Visanimation, ImportExport) {
  {
    nnl::FileReader f{GetPath("naruto_visanimation")};

    auto bin_subanim = f.ReadArrayLE<u8>(f.Len());

    auto subnaim = visanimation::Import(bin_subanim);

    auto bin_subanim_re = visanimation::Export(subnaim);

    ASSERT_TRUE(bin_subanim == bin_subanim_re);
  }
}
