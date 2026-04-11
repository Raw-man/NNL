/**
 * @file math.hpp
 * @brief Provides various math utility functions.
 *
 */
#pragma once
#include <algorithm>
#include <cassert>
#include <tuple>
#include <type_traits>
#define GLM_ENABLE_EXPERIMENTAL

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include "NNL/common/constant.hpp"
#include "NNL/common/contract.hpp"
#include "NNL/common/fixed_type.hpp"
#include "NNL/utility/trait.hpp"
namespace nnl {
/**
 * @copydoc math.hpp
 *
 */
namespace utl::math {
/**
 * \defgroup Math Math
 * @ingroup Utils
 * @copydoc math.hpp
 * @{
 */

/**
 * @brief Checks if a floating-point value is not a number.
 *
 * @param value The value to check.
 * @return true if the input value is NaN.
 */
inline bool IsNan(float value) { return std::isnan(value); }

/**
 * @brief Checks if a container has any floating-point values that are not
 * numbers.
 *
 * @tparam TContainer Container type
 * @param value The container to check.
 * @return true if the input container has any NaN values.
 */
template <typename TContainer>
bool IsNan(const TContainer& value) {
  static_assert(std::is_floating_point_v<typename TContainer::value_type>);

  return value != value;
}
/**
 * @brief Checks if a floating-point value is finite.
 *
 * @param value The value to check.
 * @return true if the input value is finite.
 */
inline bool IsFinite(float value) { return std::isfinite(value); }

/**
 * @brief Checks if all floating-point values in a container are finite.
 *
 * @tparam TContainer Container type
 * @param container The container to check.
 * @return true if all values are finite.
 */
template <typename TContainer>
bool IsFinite(const TContainer& container) {
  static_assert(std::is_floating_point_v<typename TContainer::value_type>);

  for (std::size_t i = 0; i < static_cast<std::size_t>(TContainer::length()); i++)
    if (!IsFinite(container[i])) return false;  // glm matrices, vectors

  return true;
}

/**
 * @brief Converts Euler angles in degrees (Pitch X, Yaw Y, Roll Z) to a
 * quaternion
 * @param euler The angles to convert
 * @return glm::quat
 */
glm::quat EulerToQuat(glm::vec3 euler);

/**
 * @brief Converts quaternion to Euler angles (Pitch X, Yaw Y, Roll Z)
 * @param quat The quaternion to convert
 * @return glm::vec3 containing Euler angles in _degrees_
 * @note Conversion from Euler -> Quaternion -> Euler may not yield the same set
 * of angles since there are multiple _Euler_ combinations that represent the same
 * rotation.
 */
glm::vec3 QuatToEuler(glm::quat quat);

/**
 * @brief Decomposes a transformation matrix into scale, rotation, and
 * translation
 * @param matrix The 4x4 transformation matrix to decompose
 * @return Tuple containing scale (glm::vec3), rotation (glm::quat), and
 * translation (glm::vec3)
 * @note If any scale component is 0, the rotation is set to the identity
 */
std::tuple<glm::vec3, glm::quat, glm::vec3> Decompose(const glm::mat4& matrix);

/**
 * @brief Composes a transformation matrix from scale, rotation, and
 * translation
 * @param scale
 * @param rotation
 * @param translation
 * @return A glm::mat4
 */
glm::mat4 Compose(glm::vec3 scale, glm::quat rotation, glm::vec3 translation);

/**
 * @brief Computes the inverse of a matrix safely.
 *
 * This function takes a matrix and
 * returns its inverse. If the matrix is not invertible, a small value
 * is added to its diagonal elements. If the adjusted matrix is still not
 * invertible, the function will return the identity matrix.
 *
 * @param m The matrix to be inverted.
 * @return A matrix representing the inverse or an identity matrix.
 */
glm::mat4 InverseSafe(const glm::mat4& m);

/**
 * @copydoc InverseSafe
 */
glm::mat3 InverseSafe(const glm::mat3& m);

/**
 * @brief Computes the inverse of a matrix.
 *
 * This function takes a matrix and
 * returns its inverse. If the matrix is not invertible, the result is
 * undefined.
 *
 * @param m The matrix to be inverted.
 * @return A matrix representing the inverse.
 */
glm::mat4 Inverse(const glm::mat4& m);

/**
 * @copydoc Inverse(const glm::mat4& m)
 */
glm::mat3 Inverse(const glm::mat3& m);
/**
 * @brief Normalizes a vector safely.
 *
 * Returns a unit vector pointing in the same direction as the input.
 * If the input vector has a length of zero (or is near-zero), it returns
 * a default vector.
 *
 * @param vec The vector to be normalized.
 * @return The normalized unit vector, or a default vector.
 */
glm::vec3 NormalizeSafe(const glm::vec3& vec);
/**
 * @brief Calculates the square of a value
 * @tparam T Numeric type
 * @param value The value to square
 * @return value * value
 */
template <typename T>
constexpr inline T Sqr(T value) {
  static_assert(std::is_arithmetic_v<T>);
  return value * value;
}

/**
 * @brief Checks whether a number can be expressed as an integer power of 2
 * @tparam T Integral type
 * @param n The number to check
 * @return true if n is a power of 2, false otherwise
 */
template <typename T>
bool IsPow2(T n) {
  static_assert(std::is_integral_v<T>);
  return (n > 0 && ((n & (n - 1)) == 0));
}

/**
 * @brief Rounds up the number to the next power of 2
 * @param v 32-bit unsigned integer to round
 * @return Next power of 2 or the number itself
 */
inline u32 RoundUpPow2(u32 v) {
  NNL_EXPECTS_DBG(v >= 1 && v <= 2147483648);
  v--;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  v++;
  return v;
}

/**
 * @brief Rounds down to the previous power of 2
 * @param v 32-bit unsigned integer to round
 * @return Previous power of 2 or the number itself
 */
inline u32 RoundDownPow2(u32 v) { return IsPow2(v) ? v : RoundUpPow2(v) >> 1; }

/**
 * @brief Rounds number up to the nearest multiple
 * @tparam T Integral type
 * @param number Number to round
 * @param multiple Rounding multiple
 * @return Rounded number
 */
template <typename T>
T RoundNum(T number, std::size_t multiple) {
  static_assert(std::is_integral_v<T>);

  if (multiple == 0) return number;

  if (number % multiple != 0) return ((number / multiple) + 1) * multiple;

  return number;
}

/**
 * @brief Converts a float value to a fixed point value.
 *
 * @tparam T Integral type
 * @tparam num_int Num of bits for the integer part
 * @param value Float value to convert
 * @return A fixed point value
 *
 */
template <typename T, std::size_t num_int = 0>
constexpr T FloatToFixed(f32 value) {
  static_assert(std::is_integral_v<T>);
  static_assert(num_int < std::numeric_limits<T>::digits, "no fractional bits");
  static_assert(sizeof(T) < sizeof(f32));
  NNL_EXPECTS_DBG(utl::math::IsFinite(value));

  std::size_t fraction = std::numeric_limits<T>::digits - num_int;
  f32 res = std::round(value * static_cast<f32>(1U << fraction));
  res = std::clamp<int>(res, std::numeric_limits<T>::min(), std::numeric_limits<T>::max());
  return static_cast<T>(res);
}

/**
 * @brief Converts a fixed-point value to a float value
 *
 * @tparam T Integral type
 * @tparam num_int Num of bits for the integer part
 * @param value Integer value to convert
 * @return A float value
 *
 */
template <typename T, std::size_t num_int = 0>
constexpr f32 FixedToFloat(T value) {
  static_assert(std::is_integral_v<T>);
  static_assert(num_int < std::numeric_limits<T>::digits, "no fractional bits");
  static_assert(sizeof(T) < sizeof(f32));
  std::size_t fraction = std::numeric_limits<T>::digits - num_int;
  f32 res = static_cast<f32>(value) / static_cast<f32>(1U << fraction);
  return res;
}

/**
 * @brief Checks if floating-point numbers are approximately equal
 * @param a First float
 * @param b Second float
 * @param range Tolerance range
 * @return true if |a - b| <= range
 */
inline bool IsApproxEqual(float a, float b, float range = NNL_EPSILON) {
  if (std::abs(a - b) > range) return false;
  return true;
}
/**
 * @brief Checks if two vectors or matrices are approximately equal
 * @tparam T Container type
 * @param a First value
 * @param b Second value
 * @param range Tolerance range per element
 * @return true if all elements are approximately equal
 */
template <typename TContainer>
bool IsApproxEqual(TContainer a, TContainer b, float range = NNL_EPSILON) {
  static_assert(std::is_floating_point_v<typename TContainer::value_type>);

  std::size_t size = 0;

  if constexpr (utl::trait::has_size_member_v<TContainer>) {
    if (a.size() != b.size()) return false;
    size = static_cast<std::size_t>(a.size());
  } else {
    size = static_cast<std::size_t>(TContainer::length());
  }

  for (std::size_t i = 0; i < size; i++)
    if (!IsApproxEqual(a[i], b[i], range)) return false;

  return true;
}
/** @} */
}  // namespace utl::math
}  // namespace nnl
