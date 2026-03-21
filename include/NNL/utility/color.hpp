/**
 * @file color.hpp
 * @brief Provides functions for color conversion and manipulation.
 *
 */
// Copyright (c) 2015- PPSSPP Project.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 2.0 or later versions.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License 2.0 for more details.

// A copy of the GPL 2.0 should have been included with the program.
// If not, see http://www.gnu.org/licenses/

// Official git repository and contact information can be found at
// https://github.com/hrydgard/ppsspp and http://www.ppsspp.org/.
#pragma once

#include <glm/glm.hpp>

#include "NNL/common/fixed_type.hpp"
#include "NNL/utility/data.hpp"
#include "NNL/utility/string.hpp"
namespace nnl {
/**
 * @copydoc color.hpp
 *
 */
namespace utl::color {
/**
 * \defgroup Color Color
 * @ingroup Utils
 * @copydoc color.hpp
 * @{
 */
/**
 * @brief Converts a 4-bit color channel value to an 8-bit color channel value.
 *
 * @param v The 4-bit color channel value.
 * @return The converted 8-bit color channel value.
 */
inline u8 Convert4To8(u8 v) noexcept {
  // Swizzle bits: 00001234 -> 12341234
  return (v << 4) | (v);
}

/**
 * @brief Converts a 5-bit color channel value to an 8-bit color channel value.
 *
 * @param v The 5-bit color channel value.
 * @return The converted 8-bit color channel value.
 */
inline u8 Convert5To8(u8 v) noexcept {
  // Swizzle bits: 00012345 -> 12345123
  return (v << 3) | (v >> 2);
}
/**
 * @brief Converts a 6-bit color channel value to an 8-bit color channel value.
 *
 * @param v The 6-bit color channel value.
 * @return The converted 8-bit color channel value.
 */
inline u8 Convert6To8(u8 v) noexcept {
  // Swizzle bits: 00123456 -> 12345612
  return (v << 2) | (v >> 4);
}
/**
 * @brief Converts RGBA4444 color format to RGBA8888 color format.
 *
 * @param src The source color in RGBA4444 format.
 * @return The converted color in RGBA8888 format.
 */
inline u32 RGBA4444ToRGBA8888(u16 src) noexcept {
  const u32 r = (src & 0x000F) << 0;
  const u32 g = (src & 0x00F0) << 4;
  const u32 b = (src & 0x0F00) << 8;
  const u32 a = (src & 0xF000) << 12;

  const u32 c = r | g | b | a;
  return c | (c << 4);
}

/**
 * @brief Converts RGBA5551 color format to RGBA8888 color format.
 *
 * @param src The source color in RGBA5551 format.
 * @return The converted color in RGBA8888 format.
 */
inline u32 RGBA5551ToRGBA8888(u16 src) noexcept {
  u8 r = Convert5To8((src >> 0) & 0x1F);
  u8 g = Convert5To8((src >> 5) & 0x1F);
  u8 b = Convert5To8((src >> 10) & 0x1F);
  u8 a = (src >> 15) & 0x1;
  a = (a) ? 0xff : 0;
  return (a << 24) | (b << 16) | (g << 8) | r;
}

/**
 * @brief Converts RGB565 color format to RGBA8888 color format.
 *
 * @param src The source color in RGB565 format.
 * @return The converted color in RGBA8888 format.
 */
inline u32 RGB565ToRGBA8888(u16 src) noexcept {
  u8 r = Convert5To8((src >> 0) & 0x1F);
  u8 g = Convert6To8((src >> 5) & 0x3F);
  u8 b = Convert5To8((src >> 11) & 0x1F);
  u8 a = 0xFF;
  return (a << 24) | (b << 16) | (g << 8) | r;
}
/**
 * @brief Converts RGBA8888 color format to RGB565 color format.
 *
 * @param value The source color in RGBA8888 format.
 * @return The converted color in RGB565 format.
 */
inline u16 RGBA8888ToRGB565(u32 value) noexcept {
  u32 r = (value >> 3) & 0x1F;
  u32 g = (value >> 5) & (0x3F << 5);
  u32 b = (value >> 8) & (0x1F << 11);
  return (u16)(r | g | b);
}
/**
 * @brief Converts RGBA8888 color format to RGBA5551 color format.
 *
 * @param value The source color in RGBA8888 format.
 * @return The converted color in RGBA5551 format.
 */
inline u16 RGBA8888ToRGBA5551(u32 value) noexcept {
  u32 r = (value >> 3) & 0x1F;
  u32 g = (value >> 6) & (0x1F << 5);
  u32 b = (value >> 9) & (0x1F << 10);
  u32 a = (value >> 16) & 0x8000;
  return (u16)(r | g | b | a);
}
/**
 * @brief Converts RGBA8888 color format to RGBA4444 color format.
 *
 * @param value The source color in RGBA8888 format.
 * @return The converted color in RGBA4444 format.
 */
inline u16 RGBA8888ToRGBA4444(u32 value) noexcept {
  const u32 c = value >> 4;
  const u16 r = (c >> 0) & 0x000F;
  const u16 g = (c >> 4) & 0x00F0;
  const u16 b = (c >> 8) & 0x0F00;
  const u16 a = (c >> 12) & 0xF000;
  return r | g | b | a;
}
/**
 * @brief Converts an integer color value to a hex string representation.
 *
 * @param color The 32-bit integer representing RGBA color.
 * @param alpha Boolean flag to include alpha in hex representation.
 * @return A hex string representing the color (format: "#RRGGBBAA").
 */
inline std::string IntToHex(u32 color, bool alpha = true) {
  std::string hex = "#";
  color = utl::data::SwapEndian(color);
  hex += utl::string::IntToHex(color);

  if (!alpha) hex.resize(7);

  return hex;
}
/**
 * @brief Converts a hex string representation of a color to an integer color
 * value.
 *
 * @param hex The hex string representing the color (format: "#RRGGBBAA").
 * @return A 32-bit integer representing the color in RGBA format.
 */
inline u32 HexToInt(std::string hex) {
  NNL_EXPECTS_DBG(!hex.empty() && hex.size() <= 9);  // #RRGGBBAA
  u32 color = 0;
  if (!hex.empty() && hex.at(0) == '#') hex = hex.substr(1);

  while (hex.length() < 8) hex += "f";

  color = utl::data::SwapEndian((u32)std::strtoul(hex.c_str(), nullptr, 16));

  return color;
}
/**
 * @brief Converts an integer color value to a floating-point vector
 * representation.
 *
 * @param color The 32-bit integer representing RGBA color.
 * @return A vec4 representing the color in normalized floating-point values.
 */
inline glm::vec4 IntToFloat(u32 color) noexcept {
  glm::vec4 fcolor;
  fcolor.r = (color & 0xFF) / 255.0f;
  fcolor.g = ((color >> 8) & 0xFF) / 255.0f;
  fcolor.b = ((color >> 16) & 0xFF) / 255.0f;
  fcolor.a = ((color >> 24) & 0xFF) / 255.0f;

  return fcolor;
}
/**
 * @brief Converts a floating-point color value to an integer color
 * representation.
 *
 * @param color The floating-point value representing a color component (0.0
 * to 1.0).
 * @return A 32-bit integer representing the color.
 */
inline u8 FloatToInt(float color) noexcept {
  color = std::clamp(color, 0.0f, 1.0f);
  u8 c = static_cast<u8>(std::round(color * 255.0f));
  return c;
}
/**
 * @brief Converts a vec3 floating-point color vector to an integer color
 * representation.
 *
 * @param color The vec3 color vector where components are in the range
 * [0.0, 1.0].
 * @return A 32-bit integer representing the color in RGBA format.
 */
inline u32 FloatToInt(glm::vec3 color) noexcept {
  color = glm::clamp(color, 0.0f, 1.0f);
  u32 r = (u32)(color.r * 255.0f);
  u32 g = ((u32)(color.g * 255.0f)) << 8;
  u32 b = ((u32)(color.b * 255.0f)) << 16;
  u32 a = ((u32)(0xFF)) << 24;

  return (a | b | g | r);
}
/**
 * @brief Converts a vec4 floating-point color vector to an integer color
 * representation.
 *
 * @param color The vec4 color vector where components are in the range
 * [0.0, 1.0].
 * @return A 32-bit integer representing the color in RGBA format.
 */
inline u32 FloatToInt(glm::vec4 color) {
  color = glm::clamp(color, 0.0f, 1.0f);
  u32 r = (u32)(color.r * 255.0f);
  u32 g = ((u32)(color.g * 255.0f)) << 8;
  u32 b = ((u32)(color.b * 255.0f)) << 16;
  u32 a = ((u32)(color.a * 255.0f)) << 24;

  return (a | b | g | r);
}
/**
 * @brief Converts an sRGB color value to a linear color value.
 *
 * @param c The sRGB color component.
 * @return The corresponding linear color component.
 */
inline float SRGBToLinear(float c) noexcept {
  if (c < 0.04045f)
    return (c < 0.0f) ? 0.0f : c * (1.0f / 12.92f);
  else
    return std::pow((c + 0.055f) * (1.0f / 1.055f), 2.4f);
}

/**
 * @brief Converts a vec of sRGB color values to linear color values.
 *
 * @tparam T Type of the color container (e.g., glm::vec3).
 * @param color The color container with sRGB components.
 * @return The color vector with linear components.
 */
template <typename T>
T SRGBToLinear(T color) {
  for (std::size_t i = 0; i < std::min<std::size_t>(color.length(), 3U); i++) {
    color[i] = SRGBToLinear(color[i]);
  }

  return color;
}
/**
 * @brief Converts a linear color value to an sRGB color value.
 *
 * @param c The linear color component.
 * @return The corresponding sRGB color component.
 */
inline float LinearToSRGB(float c) noexcept {
  if (c < 0.0031308f)
    return (c < 0.0f) ? 0.0f : c * 12.92f;
  else
    return 1.055f * std::pow(c, 1.0f / 2.4f) - 0.055f;
}
/**
 * @brief Converts a vector of linear color values to sRGB color values.
 *
 * @tparam T Type of the color container (e.g., glm::vec3).
 * @param color The color container with linear components.
 * @return The color container with sRGB components.
 */
template <typename T>
T LinearToSRGB(T color) {
  for (std::size_t i = 0; i < std::min<std::size_t>(color.length(), 3U); i++) {
    color[i] = LinearToSRGB(color[i]);
  }

  return color;
}
/** @} */
}  // namespace utl::color
}  // namespace nnl
