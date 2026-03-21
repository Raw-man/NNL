/**
 * @file string.hpp
 * @brief Contains various utility functions for working with strings.
 *
 */
#pragma once

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "NNL/common/contract.hpp"

namespace nnl {
/**
 * @copydoc string.hpp
 *
 */
namespace utl::string {
/**
 * \defgroup String String
 * @ingroup Utils
 * @copydoc string.hpp
 * @{
 */
/**
 * @brief Converts a number to a string padded with zero's
 * @tparam T Integral type
 * @param num The number to convert
 * @param digits Total desired length including zero padding
 * @return String with zero-padded number
 */
template <typename T>
std::string PrependZero(T num, std::size_t digits = 2) {
  static_assert(std::is_integral_v<T>, "T must be integral");
  NNL_EXPECTS_DBG(num >= 0);
  std::string str = std::to_string(num);
  return std::string(digits - std::min(digits, str.length()), '0') + str;
}
/**
 * @brief Checks if a string ends with a specific suffix
 * @param str The string to check
 * @param suffix The suffix to look for
 * @return true if str ends with suffix, false otherwise
 */
bool EndsWith(std::string_view str, std::string_view suffix);
/**
 * @brief Checks if a string starts with a specific prefix
 * @param str The string to check
 * @param prefix The prefix to look for
 * @return true if str starts with prefix, false otherwise
 */
bool StartsWith(std::string_view str, std::string_view prefix);
/**
 * @brief Joins a container of strings into one string using a delimiter
 * @param strings Container of strings to join
 * @param delim Delimiter character
 * @return Joined string
 */
std::string Join(const std::vector<std::string>& strings, std::string delim = ",");
/**
 * @brief Splits a string by delimiter
 * @param str String to split
 * @param delim Delimiter character
 * @return Vector of substrings
 */
std::vector<std::string> Split(std::string_view str, std::string delim = ",");
/**
 * @brief Truncates a string to specified length
 * @param str String to truncate
 * @param n Maximum length
 * @return Truncated string
 */
std::string Truncate(std::string_view str, std::size_t n);

/**
 * @brief Converts a float value to a string with fixed precision
 * @param value Float value to convert
 * @param n Number of decimal places
 * @return String representation
 */
std::string FloatToString(float value, int n = 6);
/**
 * @brief Converts an integer to a hexadecimal string
 * @tparam T Integral type
 * @param i Integer to convert
 * @param prefix Whether to add "0x" prefix
 * @param prepend Whether to prepend the number with leading 0's
 * @return Hexadecimal string representation
 */
template <typename T>
std::string IntToHex(T i, bool prefix = false, bool prepend = true) {
  static_assert(std::is_integral_v<T>);

  std::stringstream stream;

  if (prefix) stream << "0x";

  if (prepend) stream << std::setfill('0') << std::setw(sizeof(T) * 2);

  stream << std::hex;

  if constexpr (sizeof(i) < sizeof(int))
    stream << static_cast<int>(i);
  else
    stream << i;

  return stream.str();
}
/**
 * @brief Converts a string to lowercase
 * @param str String to convert
 * @return Lowercase string
 *
 * @note This method processes only the ASCII subset of UTF-8 characters.
 * Non-ASCII characters remain unchanged.
 */
std::string ToLower(std::string_view str);

/**
 * @brief Performs natural string comparison for sorting
 *
 * While standard alphabetical order compares strings character-by-character
 * (where "10" sorts before "2" because "1" is less than "2"), natural sort
 * order compares them by magnitude of the number, placing "2" before "10".
 *
 * @param a First string
 * @param b Second string
 * @return true if a < b using natural ordering
 */
bool CompareNat(std::string_view a, std::string_view b);

/** @} */
}  // namespace utl::string
}  // namespace nnl
