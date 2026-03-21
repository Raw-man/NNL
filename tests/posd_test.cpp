#include "NNL/game_asset/world/posd.hpp"

#include <gtest/gtest.h>

#include "test_common.hpp"
using namespace nnl;

TEST(POSD, IsOfType) {
  FileReader bin_posd(GetPath("before_tower_posd"));

  ASSERT_TRUE(posd::IsOfType(bin_posd));
}

TEST(POSD, ImportExport) {
  {
    nnl::FileReader f{GetPath("before_tower_posd")};

    auto bin_posd = f.ReadArrayLE<u8>(f.Len());

    auto pos = posd::Import(bin_posd);
    auto bin_posd_re = posd::Export(pos);

    ASSERT_TRUE(bin_posd == bin_posd_re);
  }
}

TEST(POSD, PositionToSNode) {
  nnl::FileReader f{GetPath("before_tower_posd")};

  auto bin_posd = f.ReadArrayLE<u8>(f.Len());

  auto positions = posd::Convert(posd::Import(bin_posd));

  auto bin_posd_re = posd::Export(posd::Convert(positions));

  ASSERT_TRUE(bin_posd == bin_posd_re);
}
