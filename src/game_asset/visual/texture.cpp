
#include "NNL/game_asset/visual/texture.hpp"

#include <algorithm>
#include <cmath>
#include <set>
#include <unordered_map>
#include <vector>

#include "NNL/common/io.hpp"
#include "NNL/utility/color.hpp"
#include "NNL/utility/math.hpp"
#include "stb_image.h"
#include "stb_image_resize2.h"
#include "stb_image_write.h"

namespace nnl {

namespace texture {

struct PixelF {
  float r;
  float g;
  float b;
  float a;

  operator SPixel() const {
    NNL_EXPECTS_DBG(r >= 0.0f && r <= 1.0f);
    NNL_EXPECTS_DBG(g >= 0.0f && g <= 1.0f);
    NNL_EXPECTS_DBG(b >= 0.0f && b <= 1.0f);
    NNL_EXPECTS_DBG(a >= 0.0f && a <= 1.0f);
    return SPixel{(u8)glm::round(r * 255.0f), (u8)glm::round(g * 255.0f), (u8)glm::round(b * 255.0f),
                  (u8)glm::round(a * 255.0f)};
  }
};

using namespace std::string_literals;

constexpr std::array<std::size_t, 4> clut_color_size{
    2,  // 565
    2,  // 5551
    2,  // 4444
    4   // 8888
};

constexpr std::array<std::size_t, 6> clut_color_limit{
    0,  0, 0, 0,  // no clut
    16,           // clut4
    256           // clut8
};

constexpr std::array<std::size_t, 6> pixel_size{
    2,  // 565
    2,  // 5551
    2,  // 4444
    4,  // 8888
    0,  // clut4
    1   // clut8
};

constexpr std::array clut_name{"no_clut", "no_clut", "no_clut", "no_clut", "clut4", "clut8"};

constexpr std::array color_name{"rgb565", "rgba5551", "rgba4444", "rgba8888"};

void DoUnswizzleTex16(const u8* texptr, u32* ydestp, int bxc, int byc, u32 pitch) {
  // dest_pitch The pitch (width in bytes) of the destination buffer.
  // ydestp is in 32-bits, so this is convenient.
  const u32 pitchBy32 = pitch >> 2;

  {
    const u32* src = (const u32*)texptr;
    for (int by = 0; by < byc; by++) {
      u32* xdest = ydestp;
      for (int bx = 0; bx < bxc; bx++) {
        u32* dest = xdest;
        for (int n = 0; n < 8; n++) {
          std::memcpy(dest, src, 16);
          dest += pitchBy32;
          src += 4;
        }
        xdest += 4;
      }
      ydestp += pitchBy32 * 8;
    }
  }
}

void DoSwizzleTex16(const u32* ysrcp, u8* texptr, int bxc, int byc, u32 pitch) {
  // ysrcp is in 32-bits, so this is convenient.
  const u32 pitchBy32 = pitch >> 2;

  u32* dest = (u32*)texptr;
  for (int by = 0; by < byc; by++) {
    const u32* xsrc = ysrcp;
    for (int bx = 0; bx < bxc; bx++) {
      const u32* src = xsrc;
      for (int n = 0; n < 8; n++) {
        std::memcpy(dest, src, 16);
        src += pitchBy32;
        dest += 4;
      }
      xsrc += 4;
    }
    ysrcp += pitchBy32 * 8;
  }
}

Buffer ConvertRGB565ToRGBA8888(BufferView bitmap565) {
  NNL_EXPECTS(bitmap565.size() % 2 == 0);
  u32 num_pixels = bitmap565.size() / 2;
  Buffer bitmap8888(num_pixels * sizeof(u32), 0);

  u32* ptr_bitmap8888 = (u32*)bitmap8888.data();
  u16* ptr_bitmap565 = (u16*)bitmap565.data();
  for (std::size_t i = 0; i < num_pixels; i++) {
    *ptr_bitmap8888 = utl::color::RGB565ToRGBA8888(*ptr_bitmap565);
    ptr_bitmap565++;
    ptr_bitmap8888++;
  }

  return bitmap8888;
}

Buffer ConvertRGBA5551ToRGBA8888(BufferView bitmap5551) {
  NNL_EXPECTS(bitmap5551.size() % 2 == 0);
  u32 num_pixels = bitmap5551.size() / 2;
  Buffer bitmap8888(num_pixels * sizeof(u32), 0);

  u32* ptr_bitmap8888 = (u32*)bitmap8888.data();
  u16* ptr_bitmap5551 = (u16*)bitmap5551.data();
  for (std::size_t i = 0; i < num_pixels; i++) {
    *ptr_bitmap8888 = utl::color::RGBA5551ToRGBA8888(*ptr_bitmap5551);
    ptr_bitmap5551++;
    ptr_bitmap8888++;
  }

  return bitmap8888;
}

Buffer ConvertRGBA4444ToRGBA8888(BufferView bitmap4444) {
  NNL_EXPECTS(bitmap4444.size() % 2 == 0);
  u32 num_pixels = bitmap4444.size() / 2;
  Buffer bitmap8888(num_pixels * sizeof(u32), 0);

  u32* ptr_bitmap8888 = (u32*)bitmap8888.data();
  u16* ptr_bitmap4444 = (u16*)bitmap4444.data();
  for (std::size_t i = 0; i < num_pixels; i++) {
    *ptr_bitmap8888 = utl::color::RGBA4444ToRGBA8888(*ptr_bitmap4444);
    ptr_bitmap4444++;
    ptr_bitmap8888++;
  }

  return bitmap8888;
}

Buffer ConvertRGBA8888ToRGBA4444(BufferView bitmap8888) {
  NNL_EXPECTS(bitmap8888.size() % 4 == 0);
  u32 num_pixels = bitmap8888.size() / sizeof(u32);
  Buffer bitmap4444(num_pixels * sizeof(u16), 0);

  u16* ptr_bitmap4444 = (u16*)bitmap4444.data();
  u32* ptr_bitmap8888 = (u32*)bitmap8888.data();

  for (std::size_t i = 0; i < num_pixels; i++) {
    *ptr_bitmap4444 = utl::color::RGBA8888ToRGBA4444(*ptr_bitmap8888);
    ptr_bitmap4444++;
    ptr_bitmap8888++;
  }

  return bitmap4444;
}

Buffer ConvertRGBA8888ToRGBA5551(BufferView bitmap8888) {
  NNL_EXPECTS(bitmap8888.size() % 4 == 0);
  u32 num_pixels = bitmap8888.size() / sizeof(u32);
  Buffer bitmap5551(num_pixels * sizeof(u16), 0);

  u16* ptr_bitmap5551 = (u16*)bitmap5551.data();
  u32* ptr_bitmap8888 = (u32*)bitmap8888.data();

  for (std::size_t i = 0; i < num_pixels; i++) {
    *ptr_bitmap5551 = utl::color::RGBA8888ToRGBA5551(*ptr_bitmap8888);
    ptr_bitmap5551++;
    ptr_bitmap8888++;
  }

  return bitmap5551;
}

Buffer ConvertRGBA8888ToRGB565(BufferView bitmap8888) {
  NNL_EXPECTS(bitmap8888.size() % sizeof(u32) == 0);
  u32 num_pixels = bitmap8888.size() / sizeof(u32);
  Buffer bitmap565(num_pixels * sizeof(u16), 0);

  u16* ptr_bitmap565 = (u16*)bitmap565.data();
  u32* ptr_bitmap8888 = (u32*)bitmap8888.data();

  for (std::size_t i = 0; i < num_pixels; i++) {
    *ptr_bitmap565 = utl::color::RGBA8888ToRGB565(*ptr_bitmap8888);
    ptr_bitmap565++;
    ptr_bitmap8888++;
  }

  return bitmap565;
}
// Based on the code from https://github.com/hrydgard/ppsspp
Buffer UnswizzleFromMem(BufferView texptr, u32 bufw, u32 height, u32 bytes_pixel) {
  NNL_EXPECTS(bytes_pixel <= sizeof(u32));

  NNL_EXPECTS(height != 0 && bufw != 0);

  const u32 row_width = (bytes_pixel > 0) ? (bufw * bytes_pixel) : (bufw / 2);

  NNL_EXPECTS(row_width % kMinByteBufferWidth == 0);

  NNL_EXPECTS(texptr.size() >= row_width * height);
  Buffer dest;

  dest.resize((bufw * utl::math::RoundNum(height, 8)) * sizeof(u32));

  const int bxc = row_width / 16;

  int byc = (height + 7) / 8;

  DoUnswizzleTex16(texptr.data(), (u32*)dest.data(), bxc, byc, row_width);

  dest.resize(row_width * height);

  return dest;
}
// Based on the code from https://github.com/hrydgard/ppsspp
Buffer SwizzleFromMem(Buffer& texptr, u32 bufw, u32 height, u32 bytes_pixel) {
  NNL_EXPECTS(bytes_pixel <= sizeof(u32));

  NNL_EXPECTS(utl::math::IsPow2(bufw));
  NNL_EXPECTS(height != 0 && bufw != 0);

  const u32 row_width = (bytes_pixel > 0) ? (bufw * bytes_pixel) : (bufw / 2);

  NNL_EXPECTS(row_width % kMinByteBufferWidth == 0);
  NNL_EXPECTS(texptr.size() >= row_width * height);
  Buffer dest;

  dest.resize((bufw * utl::math::RoundNum(height, 8)) * sizeof(u32));  // reserve extra space
  texptr.resize(dest.size());

  const int bxc = row_width / 16;
  int byc = (height + 7) / 8;
  if (byc == 0) byc = 1;

  DoSwizzleTex16((const u32*)texptr.data(), dest.data(), bxc, byc, row_width);

  dest.resize(row_width * utl::math::RoundNum(height, 8));

  return dest;
}

u32 QuickTexHash(BufferView data) {
  const void* checkp = data.data();
  const std::size_t size = data.size();

  NNL_EXPECTS(checkp != nullptr);
  NNL_EXPECTS(size != 0);

  u32 check = 0;

  if (((intptr_t)checkp & 0xf) == 0 && (size & 0x3f) == 0) {
    static const u16 cursor2_initial[8] = {0xc00bU, 0x9bd9U, 0x4b73U, 0xb651U, 0x4d9bU, 0x4309U, 0x0083U, 0x0001U};
    union u32x4_u16x8 {
      u32 x32[4];
      u16 x16[8];
    };
    u32x4_u16x8 cursor{};
    u32x4_u16x8 cursor2;
    static const u16 update[8] = {0x2455U, 0x2455U, 0x2455U, 0x2455U, 0x2455U, 0x2455U, 0x2455U, 0x2455U};

    for (u32 j = 0; j < 8; ++j) {
      cursor2.x16[j] = cursor2_initial[j];
    }

    const u32x4_u16x8* p = (const u32x4_u16x8*)checkp;
    for (u32 i = 0; i < size / 16; i += 4) {
      for (u32 j = 0; j < 8; ++j) {
        const u16 temp = p[i + 0].x16[j] * cursor2.x16[j];
        cursor.x16[j] += temp;
      }
      for (u32 j = 0; j < 4; ++j) {
        cursor.x32[j] ^= p[i + 1].x32[j];
        cursor.x32[j] += p[i + 2].x32[j];
      }
      for (u32 j = 0; j < 8; ++j) {
        const u16 temp = p[i + 3].x16[j] * cursor2.x16[j];
        cursor.x16[j] ^= temp;
      }
      for (u32 j = 0; j < 8; ++j) {
        cursor2.x16[j] += update[j];
      }
    }

    for (u32 j = 0; j < 4; ++j) {
      cursor.x32[j] += cursor2.x32[j];
    }
    check = cursor.x32[0] + cursor.x32[1] + cursor.x32[2] + cursor.x32[3];
  } else {
    const u32* p = (const u32*)checkp;
    for (u32 i = 0; i < size / 8; ++i) {
      check += *p++;
      check ^= *p++;
    }
  }

  return check;
}

Buffer ConvertIndexed4To8(BufferView clut4, u32 width) {
  NNL_EXPECTS(width != 0);
  NNL_EXPECTS(!clut4.empty());

  Buffer clut8(clut4.size() / ((width + 1) / 2) * width);

  assert(!clut8.empty());

  for (std::size_t i = 0, w = 0, s = 0; i < clut8.size(); i++, w++) {
    if (w == width) {
      w = 0;
      s += (width & 0b1);
    }
    auto shift = (w & 0b1) << 2;
    auto byte = clut4[(i + s) / 2];  // & 0x0F - the first index
    clut8[i] = ((0x0F << shift) & byte) >> shift;
  }

  return clut8;
}

Buffer ConvertIndexed8To4(BufferView clut8, u32 width) {
  NNL_EXPECTS(width != 0);
  NNL_EXPECTS(!clut8.empty());
  Buffer clut4(((width + 1) / 2) * (clut8.size() / width), 0);
  assert(!clut4.empty());
  for (std::size_t i = 0, w = 0, s = 0; i < clut8.size(); i++, w++) {
    if (w == width) {
      w = 0;
      s += (width & 0b1);  // skip
    }
    auto shift = (w & 0b1) << 2;
    clut4[(i + s) / 2] |= ((0x0F << shift) & (clut8[i] << shift));
    assert(clut8[i] <= 0x0F);
  }

  return clut4;
}

std::vector<SPixel> GeneratePaletteNaive(const STexture& image, std::size_t num_colors) {
  std::set<SPixel> unique_colors;

  for (auto& p : image.bitmap) {
    if (unique_colors.size() <= num_colors) unique_colors.insert(p);  // count unique colors
  }

  return std::vector<SPixel>(unique_colors.begin(), unique_colors.end());
}

void DeterminePrimaryColorSort(std::vector<SPixel>& box, u8& red_range, u8& green_range, u8& blue_range,
                               u8& alpha_range) {
  NNL_EXPECTS(!box.empty());

  auto red_comp = [](const SPixel a, const SPixel b) -> bool { return a.r < b.r; };

  auto green_comp = [](const SPixel a, const SPixel b) -> bool { return a.g < b.g; };

  auto blue_comp = [](const SPixel a, const SPixel b) -> bool { return a.b < b.b; };

  auto alpha_comp = [](const SPixel a, const SPixel b) -> bool { return a.a < b.a; };

  red_range =
      (*std::max_element(box.begin(), box.end(), red_comp)).r - (*std::min_element(box.begin(), box.end(), red_comp)).r;
  green_range = (*std::max_element(box.begin(), box.end(), green_comp)).g -
                (*std::min_element(box.begin(), box.end(), green_comp)).g;
  blue_range = (*std::max_element(box.begin(), box.end(), blue_comp)).b -
               (*std::min_element(box.begin(), box.end(), blue_comp)).b;
  alpha_range = (*std::max_element(box.begin(), box.end(), alpha_comp)).a -
                (*std::min_element(box.begin(), box.end(), alpha_comp)).a;

  if (red_range >= green_range && red_range >= blue_range && red_range >= alpha_range) {
    // std::sort(box.begin(), box.end(), red_comp);
    std::nth_element(box.begin(), box.begin() + box.size() / 2, box.end(), red_comp);
  } else if (green_range >= red_range && green_range >= blue_range && green_range >= alpha_range) {
    // std::sort(box.begin(), box.end(), green_comp);
    std::nth_element(box.begin(), box.begin() + box.size() / 2, box.end(), green_comp);
  } else if (blue_range >= red_range && blue_range >= green_range && blue_range >= alpha_range) {
    // std::sort(box.begin(), box.end(), blue_comp);
    std::nth_element(box.begin(), box.begin() + box.size() / 2, box.end(), blue_comp);
  } else {
    // std::sort(box.begin(), box.end(), alpha_comp);
    std::nth_element(box.begin(), box.begin() + box.size() / 2, box.end(), alpha_comp);
  }
}

// Based on the code from
// https://indiegamedev.net/2020/01/17/median-cut-with-floyd-steinberg-dithering-in-c/
std::vector<SPixel> GeneratePaletteMedian(const STexture& image, std::size_t num_colors) {
  NNL_EXPECTS(image.width * image.height == image.bitmap.size());
  NNL_EXPECTS(num_colors > 0);
  using Box = std::vector<SPixel>;

  using RangeBox = std::pair<u8, Box>;

  std::vector<SPixel> palette;

  std::set<SPixel> unique_colors;

  std::vector<RangeBox> boxes;

  boxes.reserve(num_colors);

  bool has_fully_transparent_bg = false;

  auto& init_box = boxes.emplace_back(RangeBox(0, {}));

  init_box.second.reserve(image.bitmap.size());

  for (auto& p : image.bitmap) {
    if (p.a > NNL_ALPHA_TRANSP) init_box.second.push_back(p);  // ignore fully transparent colors
  }

  has_fully_transparent_bg = init_box.second.size() != image.bitmap.size();

  if (has_fully_transparent_bg) {
    num_colors--;  // 1 color is reserved for the bg
  }

  while (boxes.size() < num_colors) {
    /* for each box, sort the boxes' pixels according to the color it has the
     * most range in */
    for (RangeBox& box_data : boxes) {
      u8 red_range;
      u8 green_range;
      u8 blue_range;
      u8 alpha_range;

      if (std::get<0>(box_data) == 0) {
        DeterminePrimaryColorSort(std::get<1>(box_data), red_range, green_range, blue_range, alpha_range);

        if (red_range >= green_range && red_range >= blue_range && red_range >= alpha_range) {
          std::get<0>(box_data) = red_range;
        } else if (green_range >= red_range && green_range >= blue_range && green_range >= alpha_range) {
          std::get<0>(box_data) = green_range;
        } else if (blue_range >= red_range && blue_range >= green_range && blue_range >= alpha_range) {
          std::get<0>(box_data) = blue_range;
        } else {
          std::get<0>(box_data) = alpha_range;
        }
      }
    }

    std::sort(boxes.begin(), boxes.end(),
              [](const RangeBox& a, const RangeBox& b) { return std::get<0>(a) < std::get<0>(b); });

    std::vector<RangeBox>::iterator itr = std::prev(boxes.end());
    Box& biggest_box = std::get<1>(*itr);

    // If 0, nothing to split anymore

    if (itr->first == 0) break;

    // The box is sorted already, so split at median

    Box splitB(biggest_box.begin() + biggest_box.size() / 2, biggest_box.end());

    biggest_box.resize(biggest_box.size() / 2);
    itr->first = 0;

    boxes.push_back(RangeBox(0, std::move(splitB)));
  }

  // Each box can be averaged to determine the final color

  float red_bg = 0.0f;
  float green_bg = 0.0f;
  float blue_bg = 0.0f;

  for (const RangeBox& boxData : boxes) {
    Box box = std::get<1>(boxData);
    float red_accum = 0;
    float green_accum = 0;
    float blue_accum = 0;
    float alpha_accum = 0;
    std::for_each(box.begin(), box.end(), [&](const SPixel& p) {
      red_accum += p.r;
      green_accum += p.g;
      blue_accum += p.b;
      alpha_accum += p.a;
    });

    red_accum = red_accum / static_cast<float>(box.size());
    green_accum = green_accum / static_cast<float>(box.size());
    blue_accum = blue_accum / static_cast<float>(box.size());
    alpha_accum = alpha_accum / static_cast<float>(box.size());

    red_bg += red_accum;
    green_bg += green_accum;
    blue_bg += blue_accum;

    unique_colors.insert({static_cast<u8>(std::min(std::round(red_accum), 255.0f)),
                          static_cast<u8>(std::min(std::round(green_accum), 255.0f)),
                          static_cast<u8>(std::min(std::round(blue_accum), 255.0f)),
                          static_cast<u8>(std::min(std::round(alpha_accum), 255.0f))});
  }

  if (has_fully_transparent_bg) {
    unique_colors.insert({

        static_cast<u8>(std::min(std::round(red_bg / boxes.size()), 255.0f)),
        static_cast<u8>(std::min(std::round(green_bg / boxes.size()), 255.0f)),
        static_cast<u8>(std::min(std::round(blue_bg / boxes.size()), 255.0f)), static_cast<u8>(0)});
  }

  palette.resize(unique_colors.size());

  std::copy(unique_colors.begin(), unique_colors.end(), palette.begin());

  return palette;
}

std::vector<u8> ApplyPalette(const STexture& image, const std::vector<SPixel>& palette, bool dither) {
  NNL_EXPECTS(palette.size() > 0 && palette.size() <= 256);
  NNL_EXPECTS(image.width * image.height == image.bitmap.size());

  std::size_t width = image.width;

  std::size_t height = image.height;

  const std::vector<SPixel>& destination = image.bitmap;

  Buffer indexed_image(destination.size());

  std::unordered_map<u32, std::size_t> color_map;  // color, new_color index

  std::vector<int> distances;

  distances.resize(palette.size());

  int bg_color_ind = palette.size() > 1 && palette[0].a == 0 && palette[1].a > NNL_ALPHA_TRANSP ? 0 : -1;

  if (!dither || image.width < 2 || image.height < 2) {
    for (std::size_t i = 0; i < destination.size(); i++) {
      SPixel p = destination[i];

      if (p.a <= NNL_ALPHA_TRANSP && bg_color_ind >= 0) {
        indexed_image[i] = (u8)bg_color_ind;
        continue;
      }

      u32 color = static_cast<u32>(p);

      auto itr_col_ind = color_map.find(color);

      if (itr_col_ind == color_map.end()) {
        for (std::size_t i = 0; i < palette.size(); i++) {
          const auto& a = palette[i];

          distances[i] = utl::math::Sqr((int)p.r - (int)a.r) + utl::math::Sqr((int)p.g - (int)a.g) +
                         utl::math::Sqr((int)p.b - (int)a.b) + utl::math::Sqr((int)p.a - (int)a.a);
        }

        std::size_t index = std::distance(distances.begin(), std::min_element(distances.begin(), distances.end()));
        color_map[color] = index;
        indexed_image[i] = index;
      } else {
        indexed_image[i] = itr_col_ind->second;
      }
    }
  } else {
    /* dither */
    std::vector<PixelF> larger_range(destination.size());

    for (std::size_t i = 0; i < destination.size(); i++) {
      SPixel p = destination[i];

      larger_range[i] = {static_cast<float>(p.r) / 255.f, static_cast<float>(p.g) / 255.f,
                         static_cast<float>(p.b) / 255.f, static_cast<float>(p.a) / 255.f};
    }

    for (std::size_t y = 0; y < height; ++y) {
      for (std::size_t x = 0; x < width; ++x) {
        PixelF old_px = larger_range[x + y * width];

        SPixel old_p = static_cast<SPixel>(old_px);

        if (old_p.a <= NNL_ALPHA_TRANSP && bg_color_ind >= 0) {
          indexed_image[x + y * width] = (u8)bg_color_ind;
          continue;
        }

        u32 color = static_cast<u32>(old_p);

        auto itr_col_ind = color_map.find(color);

        if (itr_col_ind == color_map.end()) {
          for (std::size_t i = 0; i < palette.size(); i++) {
            const auto& a = palette[i];

            distances[i] = utl::math::Sqr((int)old_p.r - (int)a.r) + utl::math::Sqr((int)old_p.g - (int)a.g) +
                           utl::math::Sqr((int)old_p.b - (int)a.b) + utl::math::Sqr((int)old_p.a - (int)a.a);
          }

          std::size_t index = std::distance(distances.begin(), std::min_element(distances.begin(), distances.end()));
          color_map[color] = index;

          indexed_image[x + y * width] = index;
        } else {
          indexed_image[x + y * width] = itr_col_ind->second;
        }

        SPixel new_p = palette[indexed_image[x + y * width]];

        float errorR = old_px.r - static_cast<float>(new_p.r) / 255.f;
        float errorG = old_px.g - static_cast<float>(new_p.g) / 255.f;
        float errorB = old_px.b - static_cast<float>(new_p.b) / 255.f;
        // float errorA = oldPixel.a - static_cast<float>(newPixel.a) / 255.f;

        errorR *= 0.75f;
        errorG *= 0.75f;
        errorB *= 0.75f;
        // errorA *= 0.75;

        if (x < width - 1) {
          larger_range[x + 1 + y * width].r =
              std::clamp(larger_range[x + 1 + y * width].r + errorR / 32.f * 5.f, 0.f, 1.f);
          larger_range[x + 1 + y * width].g =
              std::clamp(larger_range[x + 1 + y * width].g + errorG / 32.f * 5.f, 0.f, 1.f);
          larger_range[x + 1 + y * width].b =
              std::clamp(larger_range[x + 1 + y * width].b + errorB / 32.f * 5.f, 0.f, 1.f);
          // largerRange[x + 1 + y * width].a = std::min(std::max(largerRange[x
          // + 1 + y * width].a + errorA / 32 * 5, 0.f), 1.f);
          if (y < height - 1) {
            larger_range[x + 1 + (y + 1) * width].r =
                std::clamp(larger_range[x + 1 + (y + 1) * width].r + errorR / 32.f * 4.f, 0.f, 1.f);
            larger_range[x + 1 + (y + 1) * width].g =
                std::clamp(larger_range[x + 1 + (y + 1) * width].g + errorG / 32.f * 4.f, 0.f, 1.f);
            larger_range[x + 1 + (y + 1) * width].b =
                std::clamp(larger_range[x + 1 + (y + 1) * width].b + errorB / 32.f * 4.f, 0.f, 1.f);
            //    largerRange[x + 1 + (y + 1) * width].a =
            //    std::min(std::max(largerRange[x + 1 + (y + 1) * width].a +
            //    errorA / 32 * 4, 0.f), 1.f);
          }
          if (y < height - 2) {
            larger_range[x + 1 + (y + 2) * width].r =
                std::clamp(larger_range[x + 1 + (y + 2) * width].r + errorR / 32.f * 2.f, 0.f, 1.f);
            larger_range[x + 1 + (y + 2) * width].g =
                std::clamp(larger_range[x + 1 + (y + 2) * width].g + errorG / 32.f * 2.f, 0.f, 1.f);
            larger_range[x + 1 + (y + 2) * width].b =
                std::clamp(larger_range[x + 1 + (y + 2) * width].b + errorB / 32.f * 2.f, 0.f, 1.f);
            //  largerRange[x + 1 + (y + 2) * width].a =
            //  std::min(std::max(largerRange[x + 1 + (y + 2) * width].a +
            //  errorA / 32 * 2, 0.f), 1.f);
          }
        }
        if (x < width - 2) {
          larger_range[x + 2 + y * width].r =
              std::clamp(larger_range[x + 2 + y * width].r + errorR / 32.f * 3.f, 0.f, 1.f);
          larger_range[x + 2 + y * width].g =
              std::clamp(larger_range[x + 2 + y * width].g + errorG / 32.f * 3.f, 0.f, 1.f);
          larger_range[x + 2 + y * width].b =
              std::clamp(larger_range[x + 2 + y * width].b + errorB / 32.f * 3.f, 0.f, 1.f);
          // largerRange[x + 2 + y * width].a = std::min(std::max(largerRange[x
          // + 2 + y * width].a + errorA / 32 * 3, 0.f), 1.f);
          if (y < height - 1) {
            larger_range[x + 2 + (y + 1) * width].r =
                std::clamp(larger_range[x + 2 + (y + 1) * width].r + errorR / 32.f * 2.f, 0.f, 1.f);
            larger_range[x + 2 + (y + 1) * width].g =
                std::clamp(larger_range[x + 2 + (y + 1) * width].g + errorG / 32.f * 2.f, 0.f, 1.f);
            larger_range[x + 2 + (y + 1) * width].b =
                std::clamp(larger_range[x + 2 + (y + 1) * width].b + errorB / 32.f * 2.f, 0.f, 1.f);
            //    largerRange[x + 2 + (y + 1) * width].a =
            //    std::min(std::max(largerRange[x + 2 + (y + 1) * width].a +
            //    errorA / 32 * 2, 0.f), 1.f);
          }
        }
        if (y < height - 1) {
          larger_range[x + (y + 1) * width].r =
              std::clamp(larger_range[x + (y + 1) * width].r + errorR / 32.f * 5.f, 0.f, 1.f);
          larger_range[x + (y + 1) * width].g =
              std::clamp(larger_range[x + (y + 1) * width].g + errorG / 32.f * 5.f, 0.f, 1.f);
          larger_range[x + (y + 1) * width].b =
              std::clamp(larger_range[x + (y + 1) * width].b + errorB / 32.f * 5.f, 0.f, 1.f);
          // largerRange[x + (y + 1) * width].a =
          // std::min(std::max(largerRange[x + (y + 1) * width].a + errorA / 32
          // * 5, 0.f), 1.f);
          if (x > 0) {
            larger_range[x - 1 + (y + 1) * width].r =
                std::clamp(larger_range[x - 1 + (y + 1) * width].r + errorR / 32.f * 4.f, 0.f, 1.f);
            larger_range[x - 1 + (y + 1) * width].g =
                std::clamp(larger_range[x - 1 + (y + 1) * width].g + errorG / 32.f * 4.f, 0.f, 1.f);
            larger_range[x - 1 + (y + 1) * width].b =
                std::clamp(larger_range[x - 1 + (y + 1) * width].b + errorB / 32.f * 4.f, 0.f, 1.f);
            //     largerRange[x - 1 + (y + 1) * width].a =
            //     std::min(std::max(largerRange[x - 1 + (y + 1) * width].a +
            //     errorA / 32 * 4, 0.f), 1.f);
          }
          if (x > 1) {
            larger_range[x - 2 + (y + 1) * width].r =
                std::clamp(larger_range[x - 2 + (y + 1) * width].r + errorR / 32.f * 2.f, 0.f, 1.f);
            larger_range[x - 2 + (y + 1) * width].g =
                std::clamp(larger_range[x - 2 + (y + 1) * width].g + errorG / 32.f * 2.f, 0.f, 1.f);
            larger_range[x - 2 + (y + 1) * width].b =
                std::clamp(larger_range[x - 2 + (y + 1) * width].b + errorB / 32.f * 2.f, 0.f, 1.f);
            //    largerRange[x - 2 + (y + 1) * width].a =
            //    std::min(std::max(largerRange[x - 2 + (y + 1) * width].a +
            //    errorA / 32 * 2, 0.f), 1.f);
          }
        }
        if (y < height - 2) {
          larger_range[x + (y + 2) * width].r =
              std::clamp(larger_range[x + (y + 2) * width].r + errorR / 32.f * 3.f, 0.f, 1.f);
          larger_range[x + (y + 2) * width].g =
              std::clamp(larger_range[x + (y + 2) * width].g + errorG / 32.f * 3.f, 0.f, 1.f);
          larger_range[x + (y + 2) * width].b =
              std::clamp(larger_range[x + (y + 2) * width].b + errorB / 32.f * 3.f, 0.f, 1.f);
          // largerRange[x + (y + 2) * width].a =
          // std::min(std::max(largerRange[x + (y + 2) * width].a + errorA / 32
          // * 3, 0.f), 1.f);
          if (x > 0) {
            larger_range[x - 1 + (y + 2) * width].r =
                std::clamp(larger_range[x - 1 + (y + 2) * width].r + errorR / 32.f * 2.f, 0.f, 1.f);
            larger_range[x - 1 + (y + 2) * width].g =
                std::clamp(larger_range[x - 1 + (y + 2) * width].g + errorG / 32.f * 2.f, 0.f, 1.f);
            larger_range[x - 1 + (y + 2) * width].b =
                std::clamp(larger_range[x - 1 + (y + 2) * width].b + errorB / 32.f * 2.f, 0.f, 1.f);
            //     largerRange[x - 1 + (y + 2) * width].a =
            //     std::min(std::max(largerRange[x - 1 + (y + 2) * width].a +
            //     errorA / 32 * 2, 0.f), 1.f);
          }
        }
      }
    }
  }

  return indexed_image;
}

Buffer DealignBufferWidth(BufferView image_buffer, unsigned int width, unsigned int height, unsigned int bpp,
                          unsigned int buffer_width) {
  NNL_EXPECTS(width != 0 && height != 0);
  NNL_EXPECTS(width <= buffer_width);
  NNL_EXPECTS(bpp <= sizeof(u32));

  u32 buffer_width_bytes = bpp > 0 ? buffer_width * bpp : buffer_width / 2;

  NNL_EXPECTS(image_buffer.size() >= buffer_width_bytes * height);
  NNL_EXPECTS(buffer_width_bytes % kMinByteBufferWidth == 0);

  u32 row_width = bpp > 0 ? width * bpp : (width + 1) / 2;

  if (row_width == buffer_width_bytes) return image_buffer.CopyBuf();

  Buffer new_buffer;

  new_buffer.resize(row_width * height);

  for (std::size_t y = 0; y < height; y++) {
    for (std::size_t x = 0; x < row_width; x++) {
      std::size_t index_src = y * buffer_width_bytes + x;
      std::size_t index_dst = y * row_width + x;
      assert(index_src < image_buffer.size());
      assert(index_dst < new_buffer.size());
      new_buffer[index_dst] = image_buffer[index_src];
    }
  }

  return new_buffer;
}

Buffer AlignBufferWidth(BufferView image_buffer, unsigned int width, unsigned int height, unsigned int bpp,
                        unsigned int buffer_width) {
  NNL_EXPECTS(width != 0 && height != 0);
  NNL_EXPECTS(width <= buffer_width);
  NNL_EXPECTS(bpp <= sizeof(u32));

  u32 buffer_width_bytes = bpp > 0 ? buffer_width * bpp : buffer_width / 2;

  NNL_EXPECTS(image_buffer.size() <= buffer_width_bytes * height);
  NNL_EXPECTS(buffer_width_bytes % kMinByteBufferWidth == 0);

  u32 row_width = bpp > 0 ? width * bpp : (width + 1) / 2;

  if (row_width == buffer_width_bytes) return image_buffer.CopyBuf();

  Buffer new_buffer;

  new_buffer.resize(buffer_width_bytes * height, 0);

  for (std::size_t y = 0; y < height; y++) {
    for (std::size_t x = 0; x < row_width; x++) {
      std::size_t index_src = y * row_width + x;
      std::size_t index_dst = y * buffer_width_bytes + x;
      assert(index_src < image_buffer.size());
      assert(index_dst < new_buffer.size());
      new_buffer[index_dst] = image_buffer[index_src];
    }
  }

  return new_buffer;
}

Buffer DeindexClut8ToRGBA8888(BufferView indexed_image, BufferView palette) {
  NNL_EXPECTS(!palette.empty());
  NNL_EXPECTS(palette.size() % sizeof(u32) == 0);

  Buffer bitmap(indexed_image.size() * sizeof(u32), 0);

  u32* ptr_palette = (u32*)palette.data();
  u32* ptr_bitmap = (u32*)bitmap.data();
  u32 num_colors [[maybe_unused]] = palette.size() / sizeof(u32);

  for (auto color_code : indexed_image) {
    assert(color_code < num_colors);
    *ptr_bitmap = ptr_palette[color_code];
    ptr_bitmap++;
  }
  return bitmap;
}

Texture Convert(STexture&& stexture, const ConvertParam& tex_param) {
  NNL_EXPECTS(tex_param.max_mipmap_lvl <= kMaxMipMapLvl);

  NNL_EXPECTS(tex_param.max_width_height <= kMaxDimension);

  NNL_EXPECTS(tex_param.max_width_height >= kMinDimension);

  NNL_EXPECTS(utl::math::IsPow2(stexture.width));

  NNL_EXPECTS(!stexture.bitmap.empty());

  NNL_EXPECTS(stexture.width * stexture.height == stexture.bitmap.size());

  {
    auto new_width = stexture.width;

    auto new_height = stexture.height;

    while (new_width > tex_param.max_width_height || new_height > tex_param.max_width_height) {
      new_width /= 2;
      new_height /= 2;
    }

    if (new_width < kMinDimension || new_height < kMinDimension) {
      new_width = std::clamp(new_width, kMinDimension, kMaxDimension);
      new_height = std::clamp(new_height, kMinDimension, kMaxDimension);
    }

    stexture.Resize(new_width, new_height);
  }

  unsigned int min_width = 0;  // minimum swizzle block is 16 bytes in width

  switch (tex_param.texture_format) {
    case TextureFormat::kCLUT8:
      min_width = 16;
      break;
    case TextureFormat::kCLUT4:
      min_width = 32;
      break;
    case TextureFormat::kRGBA8888:
      min_width = 4;
      break;
    case TextureFormat::kRGBA4444:
    case TextureFormat::kRGBA5551:
    case TextureFormat::kRGB565:
      min_width = 8;
      break;
  }

  u32 tex_lvl = 0;

  std::vector<SPixel> palette;
  Buffer palette_buffer;
  const std::size_t num_colors = clut_color_limit.at(utl::data::as_int(tex_param.texture_format));
  bool should_dither = false;

  if (tex_param.texture_format >= TextureFormat::kCLUT4) {
    palette = GeneratePaletteNaive(stexture, num_colors + 1);

    if (palette.size() > num_colors) {
      should_dither = true;

      palette = GeneratePaletteMedian(stexture, num_colors);
    }

    palette.resize(num_colors, {0xFF, 0xFF, 0xFF, 0xFF});

    palette_buffer = utl::data::ReinterpretContainer<u8>(palette);

    switch (tex_param.clut_format) {
      case ClutFormat::kRGBA8888:
        break;

      case ClutFormat::kRGBA4444:
        palette_buffer = ConvertRGBA8888ToRGBA4444(palette_buffer);
        break;
      case ClutFormat::kRGBA5551:
        palette_buffer = ConvertRGBA8888ToRGBA5551(palette_buffer);
        break;
      case ClutFormat::kRGB565:
        palette_buffer = ConvertRGBA8888ToRGB565(palette_buffer);
        break;
      default:
        NNL_THROW(RuntimeError(NNL_SRCTAG("unknown color format")));
    }
  }

  utl::static_vector<TextureData, kMaxTextureLvl> texture_data;

  unsigned long new_width = stexture.width;
  unsigned long new_height = stexture.height;

  while (tex_lvl < tex_param.max_mipmap_lvl + 1 && new_width >= kMinDimension && new_height >= kMinDimension &&
         (tex_lvl == 0 || new_width >= min_width || tex_param.gen_small_mipmap)) {
    stexture.Resize(new_width, new_height);

    Buffer image_buffer;

    if (tex_param.texture_format >= TextureFormat::kCLUT4) {
      image_buffer = ApplyPalette(stexture, palette, should_dither && tex_param.clut_dither);
      if (tex_param.texture_format == TextureFormat::kCLUT4)
        image_buffer = ConvertIndexed8To4(image_buffer, stexture.width);
    } else {
      image_buffer = utl::data::ReinterpretContainer<u8>(stexture.bitmap);
    }

    switch (tex_param.texture_format) {
      case TextureFormat::kCLUT8:
        break;
      case TextureFormat::kCLUT4:
        break;
      case TextureFormat::kRGBA8888:
        break;
      case TextureFormat::kRGBA4444:

        image_buffer = ConvertRGBA8888ToRGBA4444(image_buffer);
        break;
      case TextureFormat::kRGBA5551:
        image_buffer = ConvertRGBA8888ToRGBA5551(image_buffer);
        break;
      case TextureFormat::kRGB565:
        image_buffer = ConvertRGBA8888ToRGB565(image_buffer);
        break;
    }

    if (stexture.width < min_width) {
      switch (tex_param.texture_format) {
        // buffer_width must take AT LEAST 16 bytes
        case TextureFormat::kCLUT8:
          image_buffer = AlignBufferWidth(image_buffer, stexture.width, stexture.height, 1, min_width);
          break;

        case TextureFormat::kCLUT4:
          image_buffer = AlignBufferWidth(image_buffer, stexture.width, stexture.height, 0, min_width);
          break;

        case TextureFormat::kRGBA8888:
          image_buffer = AlignBufferWidth(image_buffer, stexture.width, stexture.height, 4, min_width);
          break;

        case TextureFormat::kRGBA5551:
        case TextureFormat::kRGBA4444:
        case TextureFormat::kRGB565:
          image_buffer = AlignBufferWidth(image_buffer, stexture.width, stexture.height, 2, min_width);
          break;
      }
    }

    if (tex_param.swizzle && stexture.width > min_width) {
      switch (tex_param.texture_format) {
        case TextureFormat::kCLUT8:
          image_buffer = SwizzleFromMem(image_buffer, stexture.width, stexture.height, 1);
          break;

        case TextureFormat::kCLUT4:
          image_buffer = SwizzleFromMem(image_buffer, stexture.width, stexture.height, 0);
          break;

        case TextureFormat::kRGBA8888:
          image_buffer = SwizzleFromMem(image_buffer, stexture.width, stexture.height, 4);
          break;

        case TextureFormat::kRGBA5551:
        case TextureFormat::kRGBA4444:
        case TextureFormat::kRGB565:
          image_buffer = SwizzleFromMem(image_buffer, stexture.width, stexture.height, 2);
          break;
      }
    }

    texture_data.push_back(TextureData{(u16)stexture.width, (u16)stexture.height,
                                       (u16)std::max(stexture.width, min_width), std::move(image_buffer)});

    new_width /= 2;
    new_height /= 2;

    tex_lvl++;
  }
  assert(tex_lvl >= 1);

  NNL_ENSURES_DBG(!texture_data.empty());

  Texture texture;
  texture.clut_buffer = std::move(palette_buffer);
  texture.texture_data = std::move(texture_data);
  texture.texture_format = tex_param.texture_format;
  texture.swizzled = tex_param.swizzle;
  texture.min_filter = static_cast<TextureFilter>(stexture.min_filter);
  texture.mag_filter = static_cast<TextureFilter>(stexture.mag_filter);
  texture.clut_format = tex_param.clut_format;

  stexture = {};

  return texture;
}

STexture Convert(const Texture& texture, unsigned int level) {
  NNL_EXPECTS(level < kMaxTextureLvl);
  NNL_EXPECTS(level < texture.texture_data.size());

  const auto& texture_data = texture.texture_data.at(level);

  NNL_EXPECTS(texture_data.width != 0 && texture_data.height != 0);
  NNL_EXPECTS(texture_data.width <= texture_data.buffer_width);
  u32 px_size = pixel_size.at(utl::data::as_int(texture.texture_format));
  u32 buffer_width_bytes = px_size != 0 ? texture_data.buffer_width * px_size : texture_data.buffer_width / 2;
  u32 expected_min_buffer_size = buffer_width_bytes * texture_data.height;

  NNL_EXPECTS(texture_data.bitmap_buffer.size() >= expected_min_buffer_size);

  u32 min_width = 0;  // Minimum swizzle block is 16 bytes in width

  switch (texture.texture_format) {
    case TextureFormat::kCLUT8:
      min_width = 16;
      break;
    case TextureFormat::kCLUT4:
      min_width = 32;
      break;
    case TextureFormat::kRGBA8888:
      min_width = 4;
      break;
    case TextureFormat::kRGBA4444:
    case TextureFormat::kRGBA5551:
    case TextureFormat::kRGB565:
      min_width = 8;
      break;
  }

  assert(min_width != 0);

  Buffer image_buffer = texture_data.bitmap_buffer;
  Buffer palette = texture.clut_buffer;

  std::string name = color_name.at(utl::data::as_int(texture.clut_format)) + "_"s +
                     clut_name.at(utl::data::as_int(texture.texture_format)) + "_swizzle_" +
                     std::to_string(texture.swizzled);

  name += level > 0 ? "_lvl_" + std::to_string(level) : "";

  name += "_";

  u32 dim = ((u8)std::log2(utl::math::RoundUpPow2(texture_data.height))) << 8 |
            ((u8)std::log2(utl::math::RoundUpPow2(texture_data.buffer_width)));

  u32 clutformat = texture.texture_format >= TextureFormat::kCLUT4
                       ? 0xC5'00'00'00 | (((clut_color_limit.at(utl::data::as_int(texture.texture_format)) - 1) << 8) |
                                          (u8)texture.clut_format)
                       : 0;

  u32 cluthash = (utl::data::XXH32(palette) ^ clutformat) ^ dim;

  bool is_power2 = utl::math::IsPow2(texture_data.buffer_width) && utl::math::IsPow2(texture_data.height);
  //  Attempt to produced a texture hash for compatible with PPSSPP

  // Part 1: address (null) or bitmap hash (if the other parts aren't unique)
  name += texture.texture_format < TextureFormat::kCLUT4 && !is_power2
              ? utl::string::IntToHex(QuickTexHash(image_buffer))
              : "00000000";
  // Part 2: clut hash
  name +=
      texture.texture_format >= TextureFormat::kCLUT4 ? utl::string::IntToHex(cluthash) : utl::string::IntToHex(dim);
  // Part 3: bitmap hash
  name += is_power2 ? utl::string::IntToHex(QuickTexHash(image_buffer)) : "00000000";

  if (texture.texture_format >= TextureFormat::kCLUT4) {
    switch (texture.clut_format) {
      case ClutFormat::kRGBA8888:
        break;
      case ClutFormat::kRGBA5551:
        palette = ConvertRGBA5551ToRGBA8888(palette);
        break;
      case ClutFormat::kRGBA4444:
        palette = ConvertRGBA4444ToRGBA8888(palette);
        break;

      case ClutFormat::kRGB565:
        palette = ConvertRGB565ToRGBA8888(palette);
        break;
      default:
        NNL_THROW(RuntimeError(NNL_SRCTAG("unknown color format")));
    }
  }

  if (texture.swizzled && texture_data.width > min_width) {
    switch (texture.texture_format) {
      case TextureFormat::kCLUT8:
        image_buffer = UnswizzleFromMem(texture_data.bitmap_buffer, texture_data.buffer_width, texture_data.height, 1);
        break;
      case TextureFormat::kCLUT4:
        image_buffer = UnswizzleFromMem(texture_data.bitmap_buffer, texture_data.buffer_width, texture_data.height, 0);
        break;
      case TextureFormat::kRGBA8888:
        image_buffer = UnswizzleFromMem(texture_data.bitmap_buffer, texture_data.buffer_width, texture_data.height, 4);
        break;

      case TextureFormat::kRGBA5551:
      case TextureFormat::kRGBA4444:
      case TextureFormat::kRGB565:
        image_buffer = UnswizzleFromMem(texture_data.bitmap_buffer, texture_data.buffer_width, texture_data.height, 2);
        break;
      default:
        NNL_THROW(RuntimeError(NNL_SRCTAG("unknown texture format")));
    }
  }

  // Buffer width is at least 16 bytes
  // Remove padding
  if (texture_data.width < texture_data.buffer_width) {
    switch (texture.texture_format) {
      case TextureFormat::kCLUT8:
        image_buffer =
            DealignBufferWidth(image_buffer, texture_data.width, texture_data.height, 1, texture_data.buffer_width);
        break;

      case TextureFormat::kCLUT4:
        image_buffer =
            DealignBufferWidth(image_buffer, texture_data.width, texture_data.height, 0, texture_data.buffer_width);
        break;

      case TextureFormat::kRGBA8888:
        image_buffer =
            DealignBufferWidth(image_buffer, texture_data.width, texture_data.height, 4, texture_data.buffer_width);
        break;

      case TextureFormat::kRGBA5551:
      case TextureFormat::kRGBA4444:
      case TextureFormat::kRGB565:
        image_buffer =
            DealignBufferWidth(image_buffer, texture_data.width, texture_data.height, 2, texture_data.buffer_width);
        break;
    }
  }

  if (texture.texture_format == TextureFormat::kCLUT4) {
    image_buffer = ConvertIndexed4To8(image_buffer, texture_data.width);
  }

  if (texture.texture_format >= TextureFormat::kCLUT4) {
    image_buffer = DeindexClut8ToRGBA8888(image_buffer, palette);
  }

  switch (texture.texture_format) {
    case TextureFormat::kCLUT8:
    case TextureFormat::kCLUT4:
    case TextureFormat::kRGBA8888:

      break;
    case TextureFormat::kRGBA5551:
      image_buffer = ConvertRGBA5551ToRGBA8888(image_buffer);
      break;
    case TextureFormat::kRGBA4444:
      image_buffer = ConvertRGBA4444ToRGBA8888(image_buffer);
      break;
    case TextureFormat::kRGB565:
      image_buffer = ConvertRGB565ToRGBA8888(image_buffer);
      break;
  }

  STexture stexture;

  stexture.name = name;
  stexture.width = texture_data.width;
  stexture.height = texture_data.height;
  stexture.bitmap = utl::data::ReinterpretContainer<SPixel>(image_buffer);

  stexture.min_filter = static_cast<STextureFilter>(texture.min_filter);
  stexture.mag_filter = static_cast<STextureFilter>(texture.mag_filter);

  NNL_ENSURES_DBG(stexture.width * stexture.height == stexture.bitmap.size());

  return stexture;
}

std::vector<STexture> Convert(const TextureContainer& texture_container, bool mipmaps) {
  std::vector<STexture> stextures;

  stextures.reserve(texture_container.size() * 8);

  for (std::size_t i = 0; i < texture_container.size(); i++) {
    std::size_t levels = mipmaps ? texture_container[i].texture_data.size() : 1;

    for (std::size_t lvl = 0; lvl < levels; lvl++) {
      auto& stexture = stextures.emplace_back(Convert(texture_container[i], lvl));

      stexture.name = utl::string::PrependZero(i) + "_" + stexture.name;
    }
  }

  return stextures;
}

ConvertParam GenerateConvertParam(const Texture& texture) {
  ConvertParam param;
  param.texture_format = texture.texture_format;
  param.clut_format = texture.clut_format;
  param.clut_dither = false;
  param.max_mipmap_lvl = texture.texture_data.size() - 1;
  param.swizzle = texture.swizzled;
  return param;
}

std::vector<ConvertParam> GenerateConvertParam(const TextureContainer& texture_container) {
  std::vector<ConvertParam> params;
  params.reserve(texture_container.size());
  for (auto& texture : texture_container) {
    params.emplace_back(GenerateConvertParam(texture));
  }
  return params;
}

TextureContainer Convert_(std::vector<STexture>&& images, const std::vector<ConvertParam>& texture_params) {
  NNL_EXPECTS(texture_params.size() == images.size());

  TextureContainer texture_container;
  texture_container.resize(images.size());

  for (std::size_t i = 0; i < images.size(); i++)
    texture_container[i] = Convert(std::move(images.at(i)), texture_params.at(i));

  images.clear();

  return texture_container;
}

TextureContainer Convert(std::vector<STexture>&& images, const std::vector<ConvertParam>& texture_params) {
  return Convert_(std::move(images), texture_params);
}

TextureContainer Convert(std::vector<STexture>&& images, const ConvertParam& texture_params) {
  std::vector<ConvertParam> params(images.size(), texture_params);
  return Convert_(std::move(images), params);
}

bool IsOfType_(Reader& f) {
  using namespace raw;

  f.Seek(0);

  if (f.Len() < sizeof(RHeader)) return false;

  auto rheader = f.ReadLE<RHeader>();

  if (rheader.magic_bytes != kMagicBytes) return false;

  return true;
}

bool IsOfType(Reader& f) { return IsOfType_(f); }

bool IsOfType(BufferView buffer) { return IsOfType_(buffer); }

bool IsOfType(const std::filesystem::path& path) {
  FileReader f{path};
  return IsOfType_(f);
}

namespace raw {

TextureContainer Convert(RTextureContainer&& rtexture_container) {
  TextureContainer texture_container;

  texture_container.reserve(rtexture_container.textures.size());

  for (auto& rtexture : rtexture_container.textures) {
    auto& texture = texture_container.emplace_back();
    texture.texture_format = static_cast<TextureFormat>(rtexture.texture_format);
    texture.clut_format = static_cast<ClutFormat>(rtexture.clut_format);
    texture.swizzled = static_cast<bool>(rtexture.swizzled);
    texture.min_filter = static_cast<TextureFilter>(rtexture.filter_minifying);
    texture.mag_filter = static_cast<TextureFilter>(rtexture.filter_magnifying);

    if (rtexture.texture_format != rtexture.clut_format) {
      texture.clut_buffer =
          std::move(rtexture_container.cluts.at(rtexture_container.clut_offsets.at(rtexture.index_clut)));
    }

    for (u16 i = rtexture.index_texture_data; i < rtexture.index_texture_data + 1 + rtexture.num_texture_data_mipmaps;
         i++) {
      RTextureData& rtexture_data = rtexture_container.texture_data.at(i);

      texture.texture_data.push_back(
          TextureData{rtexture_data.width, rtexture_data.height, rtexture_data.buffer_width,
                      std::move(rtexture_container.bitmaps.at(rtexture_data.offset_bitmap))});
    }
  }

  rtexture_container = {};

  return texture_container;
}

RTextureContainer Parse(Reader& f) {
  f.Seek(0);

  RTextureContainer texture_container;

  NNL_TRY {
    texture_container.header = f.ReadLE<RHeader>();

    if (texture_container.header.magic_bytes != kMagicBytes) NNL_THROW(ParseError(NNL_SRCTAG("invalid magic bytes")));

    f.Seek(texture_container.header.offset_textures);
    // Main texture structs
    texture_container.textures = f.ReadArrayLE<RTexture>(texture_container.header.num_textures);

    f.Seek(texture_container.header.offset_texture_data);

    // Texture data structs

    texture_container.texture_data = f.ReadArrayLE<RTextureData>(texture_container.header.num_texture_data);

    f.Seek(texture_container.header.offset_clut_offsets);

    // Clut offsets
    texture_container.clut_offsets = f.ReadArrayLE<u32>(texture_container.header.num_clut_offsets);

    for (auto& texture_data : texture_container.texture_data)  // read bitmap buffers
    {
      f.Seek(texture_data.offset_bitmap);
      Buffer bitmap = f.ReadArrayLE<u8>(texture_data.size);

      texture_container.bitmaps.insert({(u32)texture_data.offset_bitmap, bitmap});
    }
    // CLUT buffers
    for (auto& rtexture : texture_container.textures) {
      // If CLUT is not used, continue
      if (static_cast<TextureFormat>(rtexture.texture_format) < TextureFormat::kCLUT4) continue;

      u32 color_size = clut_color_size.at(rtexture.clut_format);

      u32 clut_size = color_size * rtexture.clut_load_size * kColorEntriesInBlock;

      u32 clut_offset = texture_container.clut_offsets.at(rtexture.index_clut);

      f.Seek(clut_offset);

      Buffer clut = f.ReadArrayLE<u8>(clut_size);

      texture_container.cluts.insert({clut_offset, clut});
    }
  }
  NNL_CATCH(std::exception) { NNL_THROW(ParseError{NNL_SRCTAG(e.what())}); }

  return texture_container;
}

}  // namespace raw

TextureContainer Import_(Reader& f) { return raw::Convert(raw::Parse(f)); }

TextureContainer Import(Reader& f) { return Import_(f); }

TextureContainer Import(BufferView buffer) { return Import_(buffer); }

TextureContainer Import(const std::filesystem::path& path) {
  FileReader f{path};

  return Import_(f);
}

void Export_(const TextureContainer& texture_container, Writer& f) {
  using namespace raw;
  f.Seek(0);

  RHeader header;

  header.num_textures = utl::data::narrow<u16>(texture_container.size());
  header.num_texture_data = [&texture_container]() {
    std::size_t sum = 0;
    for (auto& t : texture_container) {
      sum += t.texture_data.size();
    }
    return utl::data::narrow<u16>(sum);
  }();

  header.num_clut_offsets = [&texture_container]() {
    std::size_t sum = 0;
    for (auto& t : texture_container)
      if (t.texture_format >= TextureFormat::kCLUT4) {
        sum++;
      }
    return utl::data::narrow<u16>(sum);
  }();

  auto header_off = f.WriteLE(header);

  header_off->*& RHeader::offset_textures = utl::data::narrow<u32>(f.Tell());

  auto texture_start = f.MakeOffsetLE<RTexture>();

  auto texture_start_copy = f.MakeOffsetLE<RTexture>();

  for (auto& texture : texture_container) {
    NNL_EXPECTS(!texture.texture_data.empty());
    NNL_EXPECTS(texture.texture_data.size() <= kMaxTextureLvl);
    NNL_EXPECTS(texture.texture_format < TextureFormat::kCLUT4 || !texture.clut_buffer.empty());

    RTexture rtexture;
    rtexture.texture_format = utl::data::as_int(texture.texture_format);
    rtexture.clut_load_size = 0;
    rtexture.clut_format = texture.texture_format >= TextureFormat::kCLUT4 ? utl::data::as_int(texture.clut_format)
                                                                           : utl::data::as_int(texture.texture_format);
    rtexture.filter_magnifying = utl::data::as_int(texture.mag_filter) & 1;
    rtexture.filter_minifying = utl::data::as_int(texture.min_filter) & 7;
    rtexture.swizzled = texture.swizzled;
    rtexture.num_texture_data_mipmaps = utl::data::narrow_cast<u8>(texture.texture_data.size() - 1);
    f.WriteLE(rtexture);
  }

  header_off->*& RHeader::offset_texture_data = utl::data::narrow<u32>(f.Tell());
  auto texture_data_start = f.MakeOffsetLE<RTextureData>();

  for (auto& texture : texture_container) {
    for (std::size_t tex_lvl = 0; tex_lvl < texture.texture_data.size(); tex_lvl++) {
      auto& texture_data = texture.texture_data.at(tex_lvl);
      u32 px_size = pixel_size.at(utl::data::as_int(texture.texture_format));
      u32 buffer_width_bytes = px_size != 0 ? texture_data.buffer_width * px_size : texture_data.buffer_width / 2;
      u32 expected_min_buffer_size = buffer_width_bytes * texture_data.height;
      u32 expected_max_buffer_size =
          buffer_width_bytes * utl::math::RoundUpPow2(utl::math::RoundNum(texture_data.height, 8));

      NNL_EXPECTS(utl::math::IsPow2(texture_data.width));  // few invalid assets exist
      NNL_EXPECTS(buffer_width_bytes >= kMinByteBufferWidth);
      NNL_EXPECTS(buffer_width_bytes % kMinByteBufferWidth == 0);
      NNL_EXPECTS(texture_data.buffer_width <= kMaxDimension);
      NNL_EXPECTS(texture_data.height <= kMaxDimension);
      NNL_EXPECTS(texture_data.width <= texture_data.buffer_width);
      NNL_EXPECTS(texture_data.bitmap_buffer.size() >= expected_min_buffer_size);
      NNL_EXPECTS(texture_data.bitmap_buffer.size() <= expected_max_buffer_size);
      NNL_EXPECTS(tex_lvl == 0 || texture.texture_data.at(tex_lvl - 1).width > texture_data.width);

      std::size_t color_size = clut_color_size.at(utl::data::as_int(texture.clut_format));

      NNL_EXPECTS(texture.clut_buffer.size() % color_size == 0);

// Check CLUT indices
#ifndef NDEBUG
      std::size_t num_clut_colors = texture.clut_buffer.size() / color_size;
      if (texture.texture_format == TextureFormat::kCLUT8) {
        for (std::size_t i = 0; i < texture_data.bitmap_buffer.size(); i++) {
          NNL_EXPECTS_DBG(texture_data.bitmap_buffer[i] < num_clut_colors);
        }
      }

      if (texture.texture_format == TextureFormat::kCLUT4) {
        for (std::size_t i = 0; i < texture_data.bitmap_buffer.size(); i++) {
          NNL_EXPECTS_DBG((texture_data.bitmap_buffer[i] & 0x0F) < num_clut_colors);
          NNL_EXPECTS_DBG(((texture_data.bitmap_buffer[i] & 0xF0) >> 4) < num_clut_colors);
        }
      }

#endif

      RTextureData rtexture_data;
      rtexture_data.width = texture_data.width;
      rtexture_data.height = texture_data.height;
      rtexture_data.buffer_width = texture_data.buffer_width;
      rtexture_data.size = texture_data.bitmap_buffer.size();
      f.WriteLE(rtexture_data);
    }
  }

  header_off->*& RHeader::offset_clut_offsets = utl::data::narrow<u32>(f.Tell());

  auto clut_offset_start = f.MakeOffsetLE<u32>();

  for (std::size_t i = 0; i < header.num_clut_offsets; i++) f.WriteLE((u32)0);

  if (!texture_container.empty()) f.AlignData(0x40, 0);

  {
    std::size_t i = 0;
    for (auto& texture : texture_container) {
      texture_start->*& RTexture::index_texture_data = utl::data::narrow<u16>(i);

      for (auto& texture_data : texture.texture_data) {
        texture_data_start->*& RTextureData::offset_bitmap = utl::data::narrow<u32>(f.Tell());

        f.WriteArrayLE(texture_data.bitmap_buffer);
        f.AlignData(0x10);
        ++texture_data_start;
        ++i;
      }
      ++texture_start;
    }
  }

  {
    std::size_t i = 0;
    for (auto& texture : texture_container) {
      if (texture.texture_format >= TextureFormat::kCLUT4) {
        *clut_offset_start = utl::data::narrow<u32>(f.Tell());

        f.WriteArrayLE(texture.clut_buffer);
        f.AlignData(0x10);

        std::size_t color_size = clut_color_size.at(utl::data::as_int(texture.clut_format));

        std::size_t num_clut_colors = texture.clut_buffer.size() / color_size;

        texture_start_copy->*& RTexture::index_clut = utl::data::narrow<u16>(i);
        texture_start_copy->*& RTexture::clut_load_size =
            std::max<u8>(std::ceil((float)num_clut_colors / (float)kColorEntriesInBlock), 1u);
        ++clut_offset_start;
        ++i;
      }
      ++texture_start_copy;
    }
  }
  // alignment at the end is inconsistent
  // (may depend on the num of textures)
  if (!texture_container.empty()) {
    f.AlignData(0x80);
  }
}

Buffer Export(const TextureContainer& texture_container) {
  BufferRW f;
  Export_(texture_container, f);
  return f;
}

void Export(const TextureContainer& texture_container, Writer& f) { Export_(texture_container, f); }

void Export(const TextureContainer& texture_container, const std::filesystem::path& path) {
  FileRW f{path, true};
  Export_(texture_container, f);
}

}  // namespace texture

}  // namespace nnl
