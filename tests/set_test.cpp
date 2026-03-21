#include <gtest/gtest.h>

#include "NNL/utility/static_set.hpp"

using namespace nnl;

TEST(Set, Insert) {
  utl::StaticSet<unsigned short, 8> set{8, 6, 8, 5, 4, 0, 3, 2, 1, 0, 5};

  ASSERT_EQ(set.Size(), 8);
  ASSERT_EQ(set[0], 0);
  ASSERT_EQ(set[1], 1);
  ASSERT_EQ(set[2], 2);
  ASSERT_EQ(set[3], 3);
  ASSERT_EQ(set[4], 4);
  ASSERT_EQ(set[5], 5);
  ASSERT_EQ(set[6], 6);
  ASSERT_EQ(set[7], 8);

  EXPECT_THROW(set.Insert(7), nnl::RangeError);
}

TEST(Set, Contains) {
  utl::StaticSet<unsigned short, 8> set;
  ASSERT_FALSE(set.Contains(0));
  set.Insert(0);
  ASSERT_TRUE(set.Contains(0));
  set.Insert(42);
  ASSERT_TRUE(set.Contains(42));

  ASSERT_EQ(set.Size(), 2);

  set.Clear();
  set.Insert(43);

  ASSERT_TRUE(set.Contains(43));
  ASSERT_FALSE(set.Contains(0));
  ASSERT_FALSE(set.Contains(42));
}

TEST(Set, Join) {
  utl::StaticSet<unsigned short, 5> set_a;
  utl::StaticSet<unsigned short, 5> set_b;

  set_a = set_a.Join(set_b);
  ASSERT_TRUE(set_a.IsEmpty());

  set_a.Insert(2);
  set_a.Insert(3);
  set_a.Insert(4);

  set_b.Insert(2);
  set_b.Insert(4);
  set_b.Insert(5);

  auto set_c = set_a.Join(set_b);

  ASSERT_EQ(set_c.Size(), 4);
  ASSERT_EQ(set_c[0], 2);
  ASSERT_EQ(set_c[1], 3);
  ASSERT_EQ(set_c[2], 4);
  ASSERT_EQ(set_c[3], 5);

  ASSERT_TRUE(set_b.Join(set_a) == set_a.Join(set_b));

  set_a.Clear();
  set_b.Clear();

  set_a.Insert(1);
  set_a.Insert(2);
  set_a.Insert(3);
  set_a.Insert(4);

  set_b.Insert(2);
  set_b.Insert(3);
  set_b.Insert(4);
  set_b.Insert(5);

  set_c = set_a.Join(set_b);

  ASSERT_EQ(set_c.Size(), 5);
  ASSERT_EQ(set_c[0], 1);
  ASSERT_EQ(set_c[1], 2);
  ASSERT_EQ(set_c[2], 3);
  ASSERT_EQ(set_c[3], 4);
  ASSERT_EQ(set_c[4], 5);

  ASSERT_TRUE(set_b.Join(set_a) == set_a.Join(set_b));
  ASSERT_TRUE(set_b.Join(set_b) == set_b);
  set_b.Insert(0);

  EXPECT_THROW(
      {
        auto a = set_b.Join(set_a);
        assert((void*)&a);
      },
      nnl::RangeError);
}

TEST(Set, Intersect) {
  utl::StaticSet<unsigned short, 8> set_a{2, 2, 6, 6, 1, 2, 3, 4, 5, 6};
  utl::StaticSet<unsigned short, 8> set_b{0, 2, 4, 6};

  auto set_c = set_a.Intersect(set_b);

  ASSERT_EQ(set_c.Size(), 3);

  ASSERT_EQ(set_c[0], 2);
  ASSERT_EQ(set_c[1], 4);
  ASSERT_EQ(set_c[2], 6);

  ASSERT_TRUE(set_a.Intersect(set_b) == set_b.Intersect(set_a));
}

TEST(Set, IsSubset) {
  utl::StaticSet<unsigned short, 8> set_a{1, 2, 3, 4, 5, 6};
  utl::StaticSet<unsigned short, 8> set_b{2, 4, 6};

  ASSERT_TRUE(set_a.IsSubset(set_b));
  ASSERT_FALSE(set_b.IsSubset(set_a));
  set_b.Insert(0);
  ASSERT_FALSE(set_a.IsSubset(set_b));
}

TEST(Set, Differentiate) {
  utl::StaticSet<unsigned short, 8> set_a{1, 3, 5, 7, 9};
  utl::StaticSet<unsigned short, 8> set_b{1, 4, 9};

  auto set_c = set_a.Difference(set_b);

  ASSERT_EQ(set_c[0], 3);
  ASSERT_EQ(set_c[1], 5);
  ASSERT_EQ(set_c[2], 7);
  ASSERT_EQ(set_c.Size(), 3);
}
