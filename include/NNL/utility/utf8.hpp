/**
 * @file utf8.hpp
 * @brief Provides utilities for working with the UTF-8 encoding.
 *
 */
#pragma once

#include <string>
#include <string_view>

namespace nnl {
/**
 * @copydoc utf8.hpp
 *
 */
namespace utl::utf8 {

/**
 * @brief Checks if string contains valid UTF-8 encoding
 * @param str String to validate
 * @return true if valid UTF-8, false otherwise
 */
bool IsValid(std::string_view str);
/**
 * @brief Gets size of UTF-8 character at position
 * @param str UTF-8 string
 * @param pos Position in string
 * @return Size in bytes of UTF-8 character (1-4)
 */
std::size_t GetSize(std::string_view str, std::size_t pos = 0);

/**
 * @brief Decodes UTF-8 character to Unicode code point
 * @param str UTF-8 string
 * @param pos Position in string
 * @return Unicode code point
 */
char32_t Decode(std::string_view str, std::size_t pos = 0);

/**
 * @brief Encodes Unicode code point to UTF-8
 * @param codepoint Unicode code point
 * @return UTF-8 encoded string
 */
std::string Encode(char32_t codepoint);
/**
 * @brief Checks if a Unicode code point belongs to a right-to-left script or
 * has RTL directionality.
 * @param codepoint Unicode code point
 * @return true if RTL character, false otherwise
 */
bool IsRightToLeft(char32_t codepoint);
/**
 * @brief Checks if a string contains only characters from the basic ASCII set.
 * @param str string to check
 * @return true if all characters are ASCII;
 */
bool IsASCII(std::string_view str);
}  // namespace utl::utf8
}  // namespace nnl
