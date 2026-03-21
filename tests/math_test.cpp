#include "NNL/utility/math.hpp"

#include <gtest/gtest.h>

#include <limits>
using namespace nnl;

TEST(Math, FloatToFixed) {
  auto res = utl::math::FloatToFixed<i8>(-1.0f);
  ASSERT_TRUE(-1.0f == utl::math::FixedToFloat<i8>(res));
}

TEST(Math, IsNan) {
  float nan = std::numeric_limits<float>::quiet_NaN();

  std::vector<float> v{1.0f, nan, 3.0f};
  ASSERT_FALSE(utl::math::IsNan(0.5f));
  ASSERT_TRUE(utl::math::IsNan(nan));
  ASSERT_TRUE(utl::math::IsNan(v));
}

TEST(Math, IsFinite) {
  float inf = std::numeric_limits<float>::infinity();

  ASSERT_TRUE(utl::math::IsFinite(0.5f));
  ASSERT_FALSE(utl::math::IsFinite(inf));
}

TEST(Math, SafeInverse) {
  glm::mat3 zero(0.0f);

  float det = glm::determinant(zero);

  ASSERT_TRUE(det == 0.0f);

  auto inv = utl::math::InverseSafe(zero);
  det = glm::determinant(inv);

  ASSERT_TRUE(det != 0.0f);
}

TEST(Math, NormalizeSafe) {
  glm::vec3 regular_normal{0.0f, 0.5f, 0};

  glm::vec3 res = utl::math::NormalizeSafe(regular_normal);

  ASSERT_TRUE(res.y >= 1.0f);

  glm::vec3 zero_normal{0.0f};

  res = utl::math::NormalizeSafe(zero_normal);

  ASSERT_TRUE(utl::math::IsFinite(res));

  ASSERT_TRUE(res.x != 0.0f || res.y != 0.0f || res.z != 0.0f);
}
