
#include "NNL/simple_asset/stexture.hpp"

#include <filesystem>
#include <string>
#include <vector>

#include "NNL/common/constant.hpp"
#include "NNL/common/fixed_type.hpp"
#include "NNL/utility/filesys.hpp"
#include "NNL/utility/math.hpp"
#include "stb_image.h"
#include "stb_image_resize2.h"
#include "stb_image_write.h"
namespace nnl {

using namespace std::string_literals;

void STexture::Resize(unsigned int new_width, unsigned int new_height) {
  NNL_EXPECTS(width * height == bitmap.size());

  if (width == new_width && height == new_height) return;

  std::vector<SPixel> new_bitmap8888(new_width * new_height);

  stbir_resize(bitmap.data(), width, height, 0, new_bitmap8888.data(), new_width, new_height, 0, STBIR_RGBA,
               STBIR_TYPE_UINT8_SRGB, STBIR_EDGE_CLAMP, stbir_filter::STBIR_FILTER_CUBICBSPLINE);

  width = new_width;

  height = new_height;

  bitmap = std::move(new_bitmap8888);
}

STexture STexture::Import(const std::filesystem::path& path, bool flip) {
  STexture stex;
  stex.name = utl::filesys::u8string(path);
  std::string upath = utl::filesys::u8string(path);
  int w, h, nrComponents;
  unsigned char* data = stbi_load(upath.c_str(), &w, &h, &nrComponents, 4);

  if (data == nullptr) {
    NNL_THROW(RuntimeError(NNL_SRCTAG("texture import failed: "s + upath + "\n"s + stbi_failure_reason())));
  }

  stex.bitmap.resize(w * h);
  std::memcpy(stex.bitmap.data(), data, w * h * 4);
  stex.width = w;
  stex.height = h;
  stbi_image_free(data);

  if (flip) stex.FlipV();

  return stex;
}

STexture STexture::Import(BufferView buffer, bool flip) {
  STexture stex;
  int w, h, nrComponents;
  unsigned char* data = stbi_load_from_memory(buffer.data(), buffer.Len(), &w, &h, &nrComponents, 4);
  if (data == nullptr) {
    NNL_THROW(RuntimeError(NNL_SRCTAG("texture import failed: "s + stex.name + "\n"s + stbi_failure_reason())));
  }
  stex.bitmap.resize(w * h);
  std::memcpy(stex.bitmap.data(), data, w * h * 4);
  stex.width = w;
  stex.height = h;
  stbi_image_free(data);

  if (flip) stex.FlipV();

  return stex;
}

void STexture::ExportPNG(const std::filesystem::path& path, bool flip) const {
  NNL_EXPECTS(width * height == bitmap.size());

  int result = 0;

  if (!flip) {
    result = stbi_write_png(
        utl::filesys::u8string(utl::filesys::ReplaceExtension(path, utl::filesys::u8path(".png"))).c_str(), width,
        height, 4, bitmap.data(), 0);
  } else {
    STexture flipped = *this;
    flipped.FlipV();

    result = stbi_write_png(
        utl::filesys::u8string(utl::filesys::ReplaceExtension(path, utl::filesys::u8path(".png"))).c_str(), width,
        height, 4, flipped.bitmap.data(), 0);
  }

  if (!result) {
    const char* failure_reason_c_str = stbi_failure_reason();
    std::string failure_reason = failure_reason_c_str != nullptr ? failure_reason_c_str : "";

    NNL_THROW(
        RuntimeError(NNL_SRCTAG("texture export failed: " + utl::filesys::u8string(path) + "\n" + failure_reason)));
  }
}

void stbi_write_func(void* context, void* data, int size) {
  assert(size > 0);
  assert(context != nullptr);
  assert(data != nullptr);
  auto& buffer = *reinterpret_cast<Buffer*>(context);
  buffer.resize((std::size_t)size);
  std::memcpy(buffer.data(), data, buffer.size());
}

Buffer STexture::ExportPNG(bool flip) const {
  NNL_EXPECTS(width * height == bitmap.size());

  Buffer buffer;

  int result = 0;

  if (!flip) {
    result = stbi_write_png_to_func(stbi_write_func, &buffer, width, height, 4, bitmap.data(), 0);
  } else {
    STexture flipped = *this;
    flipped.FlipV();

    result = stbi_write_png_to_func(stbi_write_func, &buffer, width, height, 4, flipped.bitmap.data(), 0);
  }

  if (!result) {
    NNL_THROW(RuntimeError(NNL_SRCTAG("texture export to memory failed: "s + name + "\n"s + stbi_failure_reason())));
  }

  return buffer;
}

void STexture::FlipV() {
  NNL_EXPECTS(width * height == bitmap.size());

  std::vector<SPixel> flipped(bitmap.size());

  for (std::size_t y = 0; y < height; y++) {
    std::size_t source_index = (height - y - 1) * width;
    std::size_t destination_index = y * width;
    std::copy(bitmap.begin() + source_index, bitmap.begin() + source_index + width,
              flipped.begin() + destination_index);
  }

  bitmap = std::move(flipped);
}

void STexture::AlignWidth() {
  NNL_EXPECTS(width * height == bitmap.size());

  std::vector<SPixel> aligned_buffer;

  auto required_width = utl::math::RoundUpPow2(width);

  if (required_width == width) return;

  aligned_buffer.reserve(required_width * height);

  for (std::size_t y = 0; y < height; y++) {
    for (std::size_t x = 0; x < width; x++) {
      aligned_buffer.push_back(bitmap.at(y * width + x));
    }

    for (std::size_t x = 0; x < required_width - width; x++) {
      aligned_buffer.push_back({0x0, 0x0, 0x0, 0x0});
    }
  }

  width = required_width;

  bitmap = std::move(aligned_buffer);
}

void STexture::AlignHeight() {
  NNL_EXPECTS(width * height == bitmap.size());

  auto required_height = utl::math::RoundUpPow2(height);

  if (required_height == height) return;

  bitmap.resize(width * required_height, {0x0, 0x0, 0x0, 0x0});

  height = required_height;
}

STexture STexture::GenerateChessPattern(unsigned int width, unsigned int height) {
  NNL_EXPECTS(utl::math::IsPow2(width) && utl::math::IsPow2(height));

  STexture stexture;

  stexture.width = width;
  stexture.height = height;

  stexture.bitmap.resize(width * height);

  unsigned int factor_width = width > height ? width / height : 1;
  unsigned int factor_height = height > width ? height / width : 1;

  for (std::size_t y = 0; y < height; y++) {
    for (std::size_t x = 0; x < width; x++) {
      u8 c = (((y & ((height / 8) * factor_width)) == 0) ^ ((x & ((width / 8) * factor_height)) == 0)) * 0xFF;

      stexture.bitmap[y * width + x] = {c, 0, c, 0xFF};
    }
  }

  return stexture;
}

void STexture::QuantizeAlpha(unsigned int levels) {
  if (levels >= 256) return;

  levels = std::max(levels, 1u) - 1;

  if (levels > 0) {
    float step = 255.0f / levels;

    for (auto& pixel : bitmap) {
      pixel.a = static_cast<u8>(std::clamp(std::round(pixel.a / step) * step, 0.0f, 255.0f));
    }
  } else {
    for (auto& pixel : bitmap) pixel.a = 255;
  }
}

bool STexture::HasAlpha() const {
  for (const auto& pixel : bitmap)
    if (pixel.a < NNL_ALPHA_OPAQ) return true;
  return false;
}

}  // namespace nnl
