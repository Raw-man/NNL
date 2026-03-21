#include <gtest/gtest.h>

#include "NNL/game_asset/container/dig.hpp"
using namespace nnl;

TEST(Compression, Compress) {
  Buffer buf{66, 66, 66, 66, 66, 66, 66, 66, 66};

  auto comp_buf = dig::Compress(buf);

  auto decomp_buf = dig::Decompress(comp_buf, buf.size());

  ASSERT_EQ(comp_buf.size(), 4);
  ASSERT_EQ(buf, decomp_buf);
}
