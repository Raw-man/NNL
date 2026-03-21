#include "NNL/game_asset/visual/lit.hpp"

#include <gtest/gtest.h>

#include "test_common.hpp"

using namespace nnl;

TEST(Lit, IsOfType) {
  FileReader bin_lit(GetPath("before_tower_lit"));

  ASSERT_TRUE(lit::IsOfType(bin_lit));
}

TEST(Lit, ImportExport) {
  {
    FileReader f{GetPath("before_tower_lit")};

    auto bin_lit = f.ReadArrayLE<u8>(f.Len());

    auto lit = lit::Import(bin_lit);
    auto bin_lit_re = lit::Export(lit);

    ASSERT_TRUE(bin_lit == bin_lit_re);
  }
}

TEST(Lit, Convert) {
  FileReader f{GetPath("before_tower_lit")};

  auto bin_lit = f.ReadArrayLE<u8>(f.Len());

  auto lit = lit::Import(bin_lit);
  auto slit = lit::Convert(lit);
  auto re_lit = lit::Convert(slit);
  auto re_bin_lit = lit::Export(re_lit);
}
