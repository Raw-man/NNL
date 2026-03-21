/**
 * @file fixed_type.hpp
 * @brief Contains macros and definitions for fixed-width types.
 */

#pragma once

#if !((defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) || __cplusplus >= 201703L)
#error "C++17 support was not detected."
#endif

#include <climits>

static_assert(CHAR_BIT == 8, "CHAR_BIT must be 8");

#include <limits>

static_assert(std::numeric_limits<float>::is_iec559, "IEEE 754 not supported");

#include <cassert>
#include <cstdint>
#include <cstring>
#include <type_traits>

/**
 * \defgroup FixedType Fixed Types
 * @ingroup Common
 * @copydoc fixed_type.hpp
 *
 * @{
 */

/**
 * @def NNL_PACK
 * @brief A structure packing directive
 *
 *  Ensures the in-memory representation of a structure matches its raw binary format.
 *
 */
#ifdef __GNUC__
#define NNL_PACK(...) __VA_ARGS__ __attribute__((__packed__))
#endif

#ifdef _MSC_VER
#define NNL_PACK(...) __pragma(pack(push, 1)) __VA_ARGS__ __pragma(pack(pop))
#endif

/** @} */

namespace nnl {
/**
 * \addtogroup FixedType
 *
 * @{
 */

using u64 = std::uint64_t;  ///< 64-bit unsigned integer
using i64 = std::int64_t;   ///< 64-bit signed integer
using f32 = float;          ///< 32-bit floating point
using u32 = std::uint32_t;  ///< 32-bit unsigned integer
using i32 = std::int32_t;   ///< 32-bit signed integer
using u16 = std::uint16_t;  ///< 16-bit unsigned integer
using i16 = std::int16_t;   ///< 16-bit signed integer
using u8 = std::uint8_t;    ///< 8-bit unsigned integer
using i8 = std::int8_t;     ///< 8-bit signed integer

/**
 * @struct Vec4
 * @brief 4D vector template with packed storage
 *
 *
 * @tparam T Component type
 */
NNL_PACK(template <typename T> struct Vec4 {
  static_assert(std::is_trivially_copyable_v<T>);
  T x, y, z, w;

  static constexpr std::size_t length() noexcept { return 4; }

  using value_type = T;

  constexpr Vec4() noexcept = default;

  constexpr Vec4(T x, T y, T z, T w) noexcept : x(x), y(y), z(z), w(w) {}

  constexpr Vec4(T val) noexcept : x(val), y(val), z(val), w(val) {}

  template <typename A>
  constexpr explicit Vec4(const Vec4<A>& vec) noexcept {
    x = static_cast<T>(vec.x);
    y = static_cast<T>(vec.y);
    z = static_cast<T>(vec.z);
    w = static_cast<T>(vec.w);
  }

  constexpr bool operator==(const Vec4& rhs) const noexcept {
    return (x == rhs.x) && (y == rhs.y) && (z == rhs.z) && (w == rhs.w);
  }

  constexpr bool operator!=(const Vec4& rhs) const noexcept { return !(this->operator==(rhs)); }

  constexpr const T& operator[](std::size_t i) const {
    assert(i < length());
    return this->*kMembers[i];
  }

  constexpr T& operator[](std::size_t i) {
    assert(i < length());
    return this->*kMembers[i];
  }

 private:
  using Members = T Vec4::*const[length()];

  static inline constexpr Members kMembers{&Vec4::x, &Vec4::y, &Vec4::z, &Vec4::w};
});
/**
 * @struct Vec3
 * @brief 3D vector template with packed storage
 *
 *
 * @tparam T Component type
 */
NNL_PACK(template <typename T> struct Vec3 {
  static_assert(std::is_trivially_copyable_v<T>);
  T x, y, z;

  static constexpr std::size_t length() noexcept { return 3; }

  using value_type = T;

  constexpr Vec3() noexcept = default;

  constexpr Vec3(T x, T y, T z) noexcept : x(x), y(y), z(z) {}

  constexpr Vec3(T val) noexcept : x(val), y(val), z(val) {}

  template <typename A>
  constexpr explicit Vec3(const Vec3<A>& vec) noexcept {
    x = static_cast<T>(vec.x);
    y = static_cast<T>(vec.y);
    z = static_cast<T>(vec.z);
  }

  constexpr bool operator==(const Vec3& rhs) const noexcept { return (x == rhs.x) && (y == rhs.y) && (z == rhs.z); }

  constexpr bool operator!=(const Vec3& rhs) const noexcept { return !(this->operator==(rhs)); }

  constexpr const T& operator[](std::size_t i) const {
    assert(i < length());
    return this->*kMembers[i];
  }

  constexpr T& operator[](std::size_t i) {
    assert(i < length());
    return this->*kMembers[i];
  }

 private:
  using Members = T Vec3::*const[length()];

  static inline constexpr Members kMembers{&Vec3::x, &Vec3::y, &Vec3::z};
});
/**
 * @struct Vec2
 * @brief 2D vector template with packed storage
 *
 *
 * @tparam T Component type
 */
NNL_PACK(template <typename T> struct Vec2 {
  static_assert(std::is_trivially_copyable_v<T>);
  T x, y;

  static constexpr std::size_t length() noexcept { return 2; }

  using value_type = T;

  constexpr Vec2() noexcept = default;

  constexpr Vec2(T x, T y) noexcept : x(x), y(y) {}

  constexpr Vec2(T val) noexcept : x(val), y(val) {}

  template <typename A>
  constexpr explicit Vec2(const Vec2<A>& vec) noexcept {
    x = static_cast<T>(vec.x);
    y = static_cast<T>(vec.y);
  }

  constexpr bool operator==(const Vec2& rhs) const noexcept { return (x == rhs.x) && (y == rhs.y); }

  constexpr bool operator!=(const Vec2& rhs) const noexcept { return !(this->operator==(rhs)); }

  constexpr const T& operator[](std::size_t i) const {
    assert(i < length());
    return this->*kMembers[i];
  }

  constexpr T& operator[](std::size_t i) {
    assert(i < length());
    return this->*kMembers[i];
  }

 private:
  using Members = T Vec2::*const[length()];

  static inline constexpr Members kMembers{&Vec2::x, &Vec2::y};
});

static_assert(sizeof(Vec4<f32>) == 0x10);

static_assert(sizeof(Vec4<u16>) == 0x8);

static_assert(sizeof(Vec4<i16>) == 0x8);

static_assert(sizeof(Vec4<u8>) == 0x4);

static_assert(sizeof(Vec4<i8>) == 0x4);

static_assert(sizeof(Vec3<f32>) == 0xC);

static_assert(sizeof(Vec3<u32>) == 0xC);

static_assert(sizeof(Vec3<u16>) == 0x6);

static_assert(sizeof(Vec3<i16>) == 0x6);

static_assert(sizeof(Vec3<i8>) == 0x3);

static_assert(sizeof(Vec3<u8>) == 0x3);

static_assert(sizeof(Vec2<f32>) == 0x8);

static_assert(sizeof(Vec2<i8>) == 0x2);

static_assert(sizeof(Vec2<u8>) == 0x2);

static_assert(sizeof(Vec2<i16>) == 0x4);

static_assert(sizeof(Vec2<u16>) == 0x4);

/**
 * @struct Mat4
 * @brief 4x4 matrix template with packed storage
 *
 * @tparam T Component type
 */
NNL_PACK(template <typename T> struct Mat4 {
  static_assert(std::is_trivially_copyable_v<T>);

  Vec4<T> r0, r1, r2, r3;

  static constexpr std::size_t length() noexcept { return 4; }

  using value_type = Vec4<T>;

  constexpr Mat4() noexcept = default;

  constexpr Mat4(Vec4<T> r0, Vec4<T> r1, Vec4<T> r2, Vec4<T> r3) noexcept : r0(r0), r1(r1), r2(r2), r3(r3) {}

  constexpr bool operator==(const Mat4& rhs) const noexcept {
    return (r0 == rhs.r0) && (r1 == rhs.r1) && (r2 == rhs.r2) && (r3 == rhs.r3);
  }

  constexpr bool operator!=(const Mat4& rhs) const noexcept { return !(*this == rhs); }

  constexpr const Vec4<T>& operator[](std::size_t i) const {
    assert(i < length());
    return this->*kMembers[i];
  }

  constexpr Vec4<T>& operator[](std::size_t i) {
    assert(i < length());
    return this->*kMembers[i];
  }

 private:
  using Members = Vec4<T> Mat4::*const[length()];

  static inline constexpr Members kMembers{&Mat4::r0, &Mat4::r1, &Mat4::r2, &Mat4::r3};
});

static_assert(sizeof(Mat4<f32>) == 0x40);

/** @} */
}  // namespace nnl
