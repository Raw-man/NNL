#include "NNL/common/io.hpp"

#include <gtest/gtest.h>

using namespace nnl;

TEST(IO, Align) {
  BufferRW a;

  a.WriteLE<u8>(1);

  a.AlignData(4);

  ASSERT_EQ(a.Len(), 4);

  a.WriteLE<u8>(1);

  a.AlignData(0x10);

  ASSERT_EQ(a.Len(), 0x10);
}

TEST(IO, Offset) {
  BufferRW b;

  b.WriteLE<u32>(0);
  b.WriteLE<u32>(1);

  auto off = b.WriteLE<u32>(0x30'30'30'30);

  auto off2 = b.WriteLE<u32>(0xFF);

  EXPECT_EQ((unsigned)off, 8);

  EXPECT_EQ((unsigned)off2, 12);
}

TEST(IO, ReadWriteContainer) {
  std::vector<std::size_t> vec(15, 1);

  BufferRW b;

  b.WriteArrayLE(vec);

  std::vector<u8> buffer = std::move(b);

  BufferView a(buffer);

  auto vec_r = a.ReadArrayLE<std::size_t>(15);

  ASSERT_EQ(vec, vec_r);
}

TEST(IO, Write) {
  BufferRW b;

  auto uval = b.WriteLE<unsigned long>(4);

  auto val = b.WriteLE<int>(-3);

  b.AlignData(sizeof(float));

  auto fval = b.WriteLE(1.0f);

  ASSERT_EQ(*uval, 4ul);

  ASSERT_EQ(*val, -3);

  ASSERT_EQ(*fval, 1.0f);
}
