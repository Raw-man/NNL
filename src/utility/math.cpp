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

  // Pick a similar set of angles if > 180 (for conversion to normalized ints)
  for (u32 i = 0; i < 3; i++) {
    if (angles[i] > 180.0f)
      angles[i] = angles[i] - 360.0f;
    else if (angles[i] < -180.0f)
      angles[i] = 360.0f + angles[i];
  }

  return angles;
}

glm::quat EulerToQuat(glm::vec3 euler) { return glm::quat(glm::radians(euler)); }

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
