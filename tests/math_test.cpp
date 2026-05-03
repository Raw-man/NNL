#include "NNL/utility/math.hpp"

#include <gtest/gtest.h>

#include <limits>

using namespace nnl;

TEST(Math, QuatEulerCompat) {
  glm::vec3 e_3{0, 136.873, 0};

  glm::vec3 e_4{0.0f, -177.5f, 0.0f};

  auto quat = utl::math::EulerToQuat(e_4);

  auto e_4_c = utl::math::QuatToEulerCompat(quat, e_3);

  ASSERT_FLOAT_EQ(e_4_c[0], e_4[0]);
  ASSERT_FLOAT_EQ(e_4_c[1], e_4[1]);
  ASSERT_FLOAT_EQ(e_4_c[2], e_4[2]);
}

TEST(Math, QuatEulerCompatLock) {
  glm::vec3 e_3{30.0f, -89.0f, 10.0f};

  glm::vec3 e_4{35.0f, -90.0f, 10.0f};

  auto quat = utl::math::EulerToQuat(e_4);

  auto e_4_c = utl::math::QuatToEulerCompat(quat, e_3);

  ASSERT_NEAR(e_4_c[0], e_4[0], 0.1f);
  ASSERT_NEAR(e_4_c[1], e_4[1], 0.1f);
  ASSERT_NEAR(e_4_c[2], e_4[2], 0.1f);
}

TEST(Math, EulerToQuatCompat) {
  glm::vec3 e_3{179, 0, 0};

  glm::vec3 e_4{-179.0f, 0, 0};

  glm::quat q_3 = utl::math::EulerToQuatCompat(e_3, glm::quat{1.0f, 0.0f, 0.0f, 0.0f});

  glm::quat q_4 = utl::math::EulerToQuatCompat(e_4, e_3);

  float res = glm::dot(q_3, q_4);

  ASSERT_TRUE(res >= 0.0f);
}

TEST(Math, NormalizeEuler) {
  glm::vec3 e{-720.0f, 350.0f, 179.999f};

  e = utl::math::NormalizeEuler(e);

  ASSERT_FLOAT_EQ(e[0], 0.0);
  ASSERT_FLOAT_EQ(e[1], -10.0f);
  ASSERT_FLOAT_EQ(e[2], 179.999f);
}

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
