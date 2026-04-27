#include "NNL/utility/math.hpp"

#include <glm/gtc/epsilon.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/matrix_decompose.hpp>

namespace nnl {

namespace utl::math {

glm::vec3 QuatToEuler(glm::quat quat) {
  NNL_EXPECTS_DBG(utl::math::IsFinite(quat));

  glm::vec3 angles = glm::degrees(glm::eulerAngles(quat));

  angles = NormalizeEuler(angles);

  return angles;
}

float UnwrapEuler(float current, float previous) {
  float diff = current - previous;

  // How many full circles the current value is away
  float circles = std::round(diff / 360.0f);

  // Reduce the distance
  float result = current - (circles * 360.0f);

  return result;
}

glm::vec3 UnwrapEuler(glm::vec3 current, glm::vec3 previous) {
  glm::vec3 result;
  result.x = UnwrapEuler(current.x, previous.x);
  result.y = UnwrapEuler(current.y, previous.y);
  result.z = UnwrapEuler(current.z, previous.z);
  return result;
}

float NormalizeEuler(float angle) { return std::remainder(angle, 360.0f); }

glm::vec3 NormalizeEuler(glm::vec3 angles) {
  angles[0] = NormalizeEuler(angles[0]);
  angles[1] = NormalizeEuler(angles[1]);
  angles[2] = NormalizeEuler(angles[2]);

  return angles;
}

glm::vec3 QuatToEulerCompat(glm::quat quat, glm::vec3 prev_euler) {
  glm::vec3 euler_a = glm::degrees(glm::eulerAngles(quat));

  glm::vec3 euler_b;
  euler_b.x = euler_a.x + 180.0f;
  euler_b.y = 180.0f - euler_a.y;
  euler_b.z = euler_a.z + 180.0f;

  euler_a = UnwrapEuler(euler_a, prev_euler);
  euler_b = UnwrapEuler(euler_b, prev_euler);

  euler_a = NormalizeEuler(euler_a);
  euler_b = NormalizeEuler(euler_b);

  glm::vec3 diff_a = glm::abs(euler_a - prev_euler);
  glm::vec3 diff_b = glm::abs(euler_b - prev_euler);

  float distance_a = diff_a.x + diff_a.y + diff_a.z;
  float distance_b = diff_b.x + diff_b.y + diff_b.z;

  if (distance_a <= distance_b)
    return euler_a;
  else
    return euler_b;
}

glm::quat EulerToQuat(glm::vec3 euler) { return glm::quat(glm::radians(euler)); }

float EulerShortLerp(float a, float b, float factor) {
  NNL_EXPECTS_DBG(factor >= 0.0f && factor <= 1.0f);

  float delta = NormalizeEuler(b - a);

  return a + delta * factor;
}

glm::vec3 EulerShortLerp(glm::vec3 a, glm::vec3 b, float factor) {
  glm::vec3 r;
  r.x = EulerShortLerp(a.x, b.x, factor);
  r.y = EulerShortLerp(a.y, b.y, factor);
  r.z = EulerShortLerp(a.z, b.z, factor);
  return r;
}

std::tuple<glm::vec3, glm::quat, glm::vec3> Decompose(const glm::mat4& m) {
  NNL_EXPECTS_DBG(utl::math::IsFinite(m));

  glm::vec3 scale;
  glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
  glm::vec3 translation = glm::vec3(m[3]);

  for (int i = 0; i < 3; i++) {
    float s = glm::sign(m[i][0] * m[i][1] * m[i][2] * m[i][3]) < 0.0f ? -1.0f : 1.0f;

    scale[i] = s * glm::length(m[i]);
  }

  if (scale.x == 0.0f || scale.y == 0.0f || scale.z == 0.0f) {
    return {scale, rotation, translation};
  }

  const glm::mat3 rotMtx{glm::vec3(m[0]) / scale[0], glm::vec3(m[1]) / scale[1], glm::vec3(m[2]) / scale[2]};
  rotation = glm::quat_cast(rotMtx);

  NNL_ENSURES_DBG(!IsNan(scale));
  NNL_ENSURES_DBG(!IsNan(rotation));
  NNL_ENSURES_DBG(!IsNan(translation));

  return {scale, rotation, translation};
}

glm::mat4 Compose(glm::vec3 scale, glm::quat rotation, glm::vec3 translation) {
  NNL_EXPECTS_DBG(utl::math::IsFinite(scale));
  NNL_EXPECTS_DBG(utl::math::IsFinite(rotation));
  NNL_EXPECTS_DBG(utl::math::IsFinite(translation));

  glm::mat4 S = glm::scale(glm::mat4(1.0f), scale);
  glm::mat4 R = glm::toMat4(rotation);
  glm::mat4 T = glm::translate(glm::mat4(1.0f), translation);

  return T * R * S;
}

template <int D, typename T>
auto Inverse_(const glm::mat<D, D, T>& m) {
  static_assert(D > 0);
#ifndef NDEBUG
  float det = glm::determinant(m);
  NNL_EXPECTS_DBG(det != 0.0f);
  NNL_EXPECTS_DBG(!utl::math::IsNan(det));
#endif

  return glm::inverse(m);
}

template <int D, typename T>
auto InverseSafe_(const glm::mat<D, D, T>& m) {
  static_assert(D > 0);
  float det = glm::determinant(m);

  if (utl::math::IsNan(det)) return glm::mat<D, D, T>(1.0f);

  if (det == 0.0f) {
    auto mc = m;
    for (int rc = 0; rc < std::min(D, 3); rc++) mc[rc][rc] += NNL_EPSILON;

    det = glm::determinant(mc);

    if (det == 0.0f) return glm::mat<D, D, T>(1.0f);

    return glm::inverse(mc);
  }

  return glm::inverse(m);
}

glm::mat3 Inverse(const glm::mat3& m) { return Inverse_(m); }

glm::mat4 Inverse(const glm::mat4& m) { return Inverse_(m); }

glm::mat4 InverseSafe(const glm::mat4& m) { return InverseSafe_(m); }

glm::mat3 InverseSafe(const glm::mat3& m) { return InverseSafe_(m); }

glm::vec3 NormalizeSafe(const glm::vec3& vec) {
  float len = glm::length2(vec);

  if (len < NNL_EPSILON2) {
    return glm::vec3(0.0, -1.0f, 0.0f);
  }

  return glm::normalize(vec);
}

}  // namespace utl::math
}  // namespace nnl
