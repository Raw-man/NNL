/**
 * @file data.hpp
 * @brief Provides functions and classes for handling binary data.
 *
 */
#pragma once

#include <array>
#include <type_traits>
#include <utility>

#include "NNL/common/fixed_type.hpp"
#include "NNL/common/io.hpp"
namespace nnl {
/**
 * @copydoc data.hpp
 *
 */
namespace utl::data {

/**
 * \defgroup Data Data
 * @ingroup Utils
 * @copydoc data.hpp
 * @{
 */

/**
 * @brief Generates a numeric four character code from a given string.
 *
 * Combines character values of the input string into a single unsigned integer.
 *
 * @param str Input string.
 * @return A u32 code.
 */
constexpr u32 FourCC(const char (&str)[5]) noexcept {
  u32 result{0};
  for (std::size_t i = 0; i < 4; ++i) {
    result |= static_cast<u32>(static_cast<u8>(str[i])) << (i * CHAR_BIT);
  }
  return result;
}

/**
 * @brief Swaps endianness of a value
 * @tparam T Numeric type
 * @param src Value to swap
 * @return Value with swapped endianness
 */
template <typename T>
T SwapEndian(T src) noexcept {
  static_assert(std::is_arithmetic_v<T>);
  T swapped;

  for (std::size_t i = 0; i < sizeof(T); i++)
    reinterpret_cast<unsigned char*>(&swapped)[i] = reinterpret_cast<unsigned char*>(&src)[sizeof(T) - i - 1];

  return swapped;
}

/**
 * @brief Converts an enum value to its underlying integer type
 * @tparam E Enum type
 * @param e Enum value
 * @return The underlying integer value of the enum
 */
template <typename E>
constexpr typename std::underlying_type<E>::type as_int(E e) noexcept {
  return static_cast<typename std::underlying_type<E>::type>(e);
}

/**
 * @brief Narrowing cast **without** bounds checking
 *
 * This is a semantic alias for `static_cast`. It signals intent that a loss
 * of precision is expected and handled.
 *
 * @tparam T Destination type
 * @tparam U Source type
 * @param u Value to cast
 * @return Narrowed value of type T
 *
 * @see nnl::utl::data::narrow
 */
template <class T, class U>
constexpr T narrow_cast(U&& u) noexcept {
  return static_cast<T>(std::forward<U>(u));
}

/**
 * @brief Safe narrowing cast with runtime bounds checking
 *
 * @tparam T Destination type
 * @tparam U Source type
 * @param u Value to cast
 * @return Narrowed value of type T
 * @throws NarrowError if narrowing would lose information
 *
 * @see nnl::utl::data::narrow_cast
 */
template <class T, class U, typename std::enable_if<std::is_arithmetic<T>::value>::type* = nullptr>
constexpr T narrow(U u) {
  constexpr const bool is_different_signedness = (std::is_signed<T>::value != std::is_signed<U>::value);

  const T t = utl::data::narrow_cast<T>(u);

#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif
  if (static_cast<U>(t) != u || (is_different_signedness && ((t < T{}) != (u < U{})))) {
    NNL_THROW(NarrowError(""));
  }
#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

  return t;
}

/**
 * @brief Calculates a CRC32 checksum
 * @param data Data to digest
 * @param polynomial Custom polynomial
 * @return CRC32 checksum
 */
u32 CRC32(BufferView data, u32 polynomial = 0xEDB88320) noexcept;

/**
 * @brief Calculates an XXH32 checksum
 * @param data Data to digest
 * @param seed Custom seed
 * @return XXH32 checksum
 */
u32 XXH32(BufferView data, u32 seed = 0xC0108888);

/**
 * @class MD5Context
 * @brief A utility class for calculating MD5 message digests.
 *
 * This class provides a stateful interface for computing 128-bit hashes using the MD5 algorithm.
 * It follows the "update-final" pattern, allowing datasets to be processed
 * in chunks.
 */
class MD5Context {
 public:
  /**
   * @brief Updates the hash state by processing a new chunk of data.
   *
   * @param data The input data to be hashed.
   */
  void Update(BufferView data) noexcept;
  /**
   * @brief Finalizes the hashing process and retrieves the result.
   *
   * @return A 16-byte array containing the resulting message digest.
   */
  [[nodiscard]] std::array<u8, 16> Final() noexcept;

 private:
  std::size_t lo_ = 0, hi_ = 0;
  std::size_t a_ = 0x67452301;
  std::size_t b_ = 0xefcdab89;
  std::size_t c_ = 0x98badcfe;
  std::size_t d_ = 0x10325476;
  unsigned char buffer_[64] = {};
  std::size_t block_[16] = {};

  const unsigned char* Body(const unsigned char* data, std::size_t size);
};

/**
 * @brief Calculates an MD5 checksum
 *
 * @code
 * std::vector<unsigned char> buffer(16,0);
 * auto digest = utl::data::MD5(buffer);
 * // auto digest = utl::data::MD5({buffer.data(), buffer.size()*sizeof(unsigned char)});
 * @endcode
 *
 * @param data Data to digest
 * @return MD5 checksum
 */
std::array<u8, 16> MD5(BufferView data) noexcept;
/**
 * @brief Reinterprets container data as a different type (via a bitwise copy)
 * @tparam To Destination type
 * @tparam From Source type
 * @param container Source container
 * @return New container with reinterpreted data
 *
 * @throws RuntimeError if the byte size of containers is not the same
 */
template <typename To, typename From>
auto ReinterpretContainer(const std::vector<From>& container) {
  static_assert(std::is_trivially_copyable_v<From>, "From type must be trivially copyable");
  static_assert(std::is_trivially_copyable_v<To>, "To type must be trivially copyable");

  std::size_t new_size = (container.size() * sizeof(From)) / sizeof(To);

  if (new_size * sizeof(To) != container.size() * sizeof(From)) {
    NNL_THROW(RuntimeError(NNL_SRCTAG("size mismatch")));
  }

  std::vector<To> new_container(new_size);

  std::memcpy(new_container.data(), container.data(), new_container.size() * sizeof(To));

  return new_container;
}
/**
 * @brief Casts container elements to a different type
 * @tparam To Destination type
 * @tparam From Source type
 * @param container Source container
 * @return New container with casted elements
 */
template <typename To, typename From>
auto CastContainer(const std::vector<From>& container) {
  std::vector<To> new_container(container.size());

  for (std::size_t i = 0; i < container.size(); i++) {
    new_container[i] = static_cast<To>(container[i]);
  }

  return new_container;
}

/** @} */
}  // namespace utl::data
}  // namespace nnl
