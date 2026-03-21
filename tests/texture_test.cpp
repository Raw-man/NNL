#include "NNL/game_asset/visual/texture.hpp"

#include <gtest/gtest.h>

#include "test_common.hpp"
using namespace nnl;

TEST(Texture, IsOfType) {
  FileReader bin_texture(GetPath("naruto_texture"));

  ASSERT_TRUE(texture::IsOfType(bin_texture));
}

TEST(Texture, ConvertIndexed8To4) {
  const Buffer buffer{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF};

  const Buffer expected_result_buffer{0x10, 0x32, 0x54, 0x76, 0x98, 0xBA, 0xDC, 0xFE};

  auto result_buffer = texture::ConvertIndexed8To4(buffer, 2);

  ASSERT_EQ(result_buffer.size(), expected_result_buffer.size());

  ASSERT_TRUE(result_buffer == expected_result_buffer);
}

TEST(Texture, ImportExport) {
  {
    nnl::FileReader f{GetPath("naruto_texture")};
    auto bin_asset = f.ReadArrayLE<u8>(f.Len());
    auto texture_archive = texture::Import(bin_asset);
    auto bin_texture_re = texture::Export(texture_archive);

    bin_texture_re.resize(bin_asset.size(), 0);  // padding may mismatch

    ASSERT_TRUE(bin_asset == bin_texture_re);
  }
}

TEST(Texture, ConvertSTextureToTexture) {
  const unsigned int width = 512;
  const unsigned int height = 256;
  STexture stexture;

  stexture.name = "tmp";

  stexture.width = width;

  stexture.height = height;

  stexture.bitmap.resize(stexture.width * stexture.height, {0, 0xFF, 0, 0xFF});

  texture::ConvertParam p;
  p.gen_small_mipmap = true;

  texture::Texture texture = texture::Convert(std::move(stexture), p);

  ASSERT_EQ(texture.texture_data.size(), 1 + texture::kMaxMipMapLvl);

  ASSERT_EQ(texture.texture_data[0].width, width);
  ASSERT_EQ(texture.texture_data[0].height, height);
  ASSERT_EQ(texture.texture_data[0].buffer_width, width);
  ASSERT_EQ(texture.texture_data[0].bitmap_buffer.size(), width * height);

  ASSERT_EQ(texture.texture_data[texture::kMaxMipMapLvl].width, width >> texture::kMaxMipMapLvl);
  ASSERT_EQ(texture.texture_data[texture::kMaxMipMapLvl].height, height >> texture::kMaxMipMapLvl);
  ASSERT_EQ(texture.texture_data[texture::kMaxMipMapLvl].buffer_width, texture::kMinByteBufferWidth);

  ASSERT_EQ(texture.texture_data[texture::kMaxMipMapLvl].bitmap_buffer.size(),
            texture::kMinByteBufferWidth * (height >> texture::kMaxMipMapLvl));

  ASSERT_EQ(texture.clut_buffer.size(), 256 * 4);

  ASSERT_EQ(texture.clut_buffer[0], 0);
  ASSERT_EQ(texture.clut_buffer[1], 0xFF);
  ASSERT_EQ(texture.clut_buffer[2], 0);
  ASSERT_EQ(texture.clut_buffer[3], 0xFF);
}
