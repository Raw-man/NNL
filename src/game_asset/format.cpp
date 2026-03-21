#include "NNL/game_asset/format.hpp"

#include "NNL/game_asset/audio/phd.hpp"
#include "NNL/game_asset/behavior/action.hpp"
#include "NNL/game_asset/container/asset.hpp"
#include "NNL/game_asset/container/collection.hpp"
#include "NNL/game_asset/container/dig.hpp"
#include "NNL/game_asset/container/dig_entry.hpp"
#include "NNL/game_asset/interaction/colbox.hpp"
#include "NNL/game_asset/interaction/collision.hpp"
#include "NNL/game_asset/interaction/shadow_collision.hpp"
#include "NNL/game_asset/visual/animation.hpp"
#include "NNL/game_asset/visual/fog.hpp"
#include "NNL/game_asset/visual/lit.hpp"
#include "NNL/game_asset/visual/minimap.hpp"
#include "NNL/game_asset/visual/model.hpp"
#include "NNL/game_asset/visual/render.hpp"
#include "NNL/game_asset/visual/text.hpp"
#include "NNL/game_asset/visual/texture.hpp"
#include "NNL/game_asset/visual/ui.hpp"
#include "NNL/game_asset/visual/visanimation.hpp"
#include "NNL/game_asset/world/posd.hpp"
namespace nnl {

namespace format {

bool IsPNG_(Reader& f) {
  f.Seek(0);
  if (f.Len() < sizeof(u64)) return false;

  u64 magic = f.ReadLE<u64>();

  if (magic != 0xA1A0A0D474E5089UL) return false;

  return true;
}

bool IsPGD_(Reader& f) {
  f.Seek(0);

  if (f.Len() < 0x90) return false;  // PGD header

  u32 magic = f.ReadLE<u32>();

  if (magic != utl::data::FourCC("\0PGD")) return false;

  u32 version = f.ReadLE<u32>();

  if (version > 3) return false;

  return true;
}

bool IsCCSF_(Reader& f) {
  f.Seek(0);

  if (f.Len() < 0x34) return false;

  u32 type = f.ReadLE<u32>();

  if (type != 0xCCCC0001) return false;

  u32 size = f.ReadLE<u32>();

  if (size != 0xD) return false;

  u32 block_id = f.ReadLE<u32>();

  if (block_id != utl::data::FourCC("CCSF")) return false;

  return true;
}

bool IsPSPELF_(Reader& f) {
  f.Seek(0);

  if (f.Len() < 0x10) return false;

  u32 magic = f.ReadLE<u32>();

  if (magic != utl::data::FourCC("~PSP")) return false;

  return true;
}

bool IsELF_(Reader& f) {
  f.Seek(0);

  if (f.Len() < 0x10) return false;

  u32 magic = f.ReadLE<u32>();

  if (magic != utl::data::FourCC("\x7F"
                                 "ELF"))
    return false;

  return true;
}

bool IsPSF_(Reader& f) {
  f.Seek(0);

  if (f.Len() < 0x10) return false;

  u32 magic = f.ReadLE<u32>();

  if (magic != utl::data::FourCC("\0PSF")) return false;

  return true;
}
// UTF-8 encoding
bool IsPlainText_(Reader& str) {
  str.Seek(0);

  for (std::size_t i = 0, ix = str.Len(); i < ix; i++) {
    assert(i == str.Tell());

    unsigned char c = str.ReadLE<unsigned char>();
    std::size_t n = 0;

    if ((c < 32 && c != '\t' && c != '\n' && c != '\r') || c == 127) return false;

    unsigned char c_next = 0;

    if (i + 1 != ix) c_next = str.ReadLE<unsigned char>();

    if (c == '\r' && c_next != '\n') return false;

    if (c <= 0x7f)
      n = 0;
    else if ((c & 0xE0) == 0xC0)
      n = 1;  // 110bbbbb
    else if (c == 0xed && (c_next & 0xa0) == 0xa0)
      return false;  // U+d800 to U+dfff
    else if ((c & 0xF0) == 0xE0)
      n = 2;  // 1110bbbb
    else if ((c & 0xF8) == 0xF0)
      n = 3;  // 11110bbb
    else
      return false;

    str.Seek(i + 1);

    // n bytes matching 10bbbbbb follow ?
    for (std::size_t j = 0; j < n && i < ix; j++) {
      if ((++i == ix) || ((str.ReadLE<unsigned char>() & 0xC0) != 0x80)) return false;
    }
  }
  return true;
}

bool IsAT3_(Reader& f) {
  f.Seek(0);

  if (f.Len() < sizeof(u32)) return false;

  auto magic_bytes = f.ReadLE<u32>();

  if (magic_bytes != utl::data::FourCC("RIFF")) return false;

  return true;
}

FileFormat Detect_(Reader& f) {
  f.Seek(0);

  if (f.Len() < sizeof(u32)) return FileFormat::kUnknown;

  // Formats that use magic bytes:

  if (model::IsOfType(f)) return FileFormat::kModel;

  if (texture::IsOfType(f)) return FileFormat::kTextureContainer;

  if (animation::IsOfType(f)) return FileFormat::kAnimationContainer;

  if (fog::IsOfType(f)) return FileFormat::kFog;

  if (posd::IsOfType(f)) return FileFormat::kPositionData;

  if (lit::IsOfType(f)) return FileFormat::kLit;

  if (phd::IsOfType(f)) return FileFormat::kPHD;

  if (colbox::IsOfType(f)) return FileFormat::kColboxConfig;

  if (text::IsOfType(f)) return FileFormat::kText;

  if (ui::IsOfType(f)) return FileFormat::kUIConfig;

  if (render::IsOfType(f)) return FileFormat::kRenderConfig;

  if (IsAT3_(f)) return FileFormat::kATRAC3;

  if (IsPNG_(f)) return FileFormat::kPNG;

  if (IsPGD_(f)) return FileFormat::kPGD;

  if (IsCCSF_(f)) return FileFormat::kCCSF;

  if (IsPSPELF_(f)) return FileFormat::kPSPELF;

  if (IsELF_(f)) return FileFormat::kELF;

  if (IsPSF_(f)) return FileFormat::kPSF;

  // No magic bytes:

  if (dig::IsOfType(f)) return FileFormat::kDig;

  if (dig_entry::IsOfType(f)) return FileFormat::kDigEntry;

  if (asset::IsOfType(f)) return FileFormat::kAssetContainer;

  if (collection::IsOfType(f)) return FileFormat::kCollection;

  if (collision::IsOfType(f)) return FileFormat::kCollision;

  if (shadow_collision::IsOfType(f)) return FileFormat::kShadowCollision;

  if (action::IsOfType(f)) return FileFormat::kActionConfig;

  if (visanimation::IsOfType(f)) return FileFormat::kVisanimationContainer;

  if (IsPlainText_(f)) return FileFormat::kPlainText;

  if (minimap::IsOfType(f)) return FileFormat::kMinimapConfig;

  return FileFormat::kUnknown;
}

std::vector<FileFormat> DetectAll_(Reader& f) {
  f.Seek(0);

  std::vector<FileFormat> types;

  if (f.Len() < sizeof(u32)) return types;

  // Formats that use magic bytes:

  if (model::IsOfType(f)) types.push_back(FileFormat::kModel);

  if (texture::IsOfType(f)) types.push_back(FileFormat::kTextureContainer);

  if (animation::IsOfType(f)) types.push_back(FileFormat::kAnimationContainer);

  if (fog::IsOfType(f)) types.push_back(FileFormat::kFog);

  if (posd::IsOfType(f)) types.push_back(FileFormat::kPositionData);

  if (lit::IsOfType(f)) types.push_back(FileFormat::kLit);

  if (phd::IsOfType(f)) types.push_back(FileFormat::kPHD);

  if (colbox::IsOfType(f)) types.push_back(FileFormat::kColboxConfig);

  if (text::IsOfType(f)) types.push_back(FileFormat::kText);

  if (ui::IsOfType(f)) types.push_back(FileFormat::kUIConfig);

  if (render::IsOfType(f)) types.push_back(FileFormat::kRenderConfig);

  if (IsAT3_(f)) types.push_back(FileFormat::kATRAC3);

  if (IsPNG_(f)) types.push_back(FileFormat::kPNG);

  if (IsPGD_(f)) types.push_back(FileFormat::kPGD);

  if (IsCCSF_(f)) types.push_back(FileFormat::kCCSF);

  if (IsPSPELF_(f)) types.push_back(FileFormat::kPSPELF);

  if (IsELF_(f)) types.push_back(FileFormat::kELF);

  if (IsPSF_(f)) types.push_back(FileFormat::kPSF);

  // No magic bytes:

  if (dig::IsOfType(f)) types.push_back(FileFormat::kDig);

  if (dig_entry::IsOfType(f)) types.push_back(FileFormat::kDigEntry);

  if (asset::IsOfType(f)) types.push_back(FileFormat::kAssetContainer);

  if (collection::IsOfType(f)) types.push_back(FileFormat::kCollection);

  if (collision::IsOfType(f)) types.push_back(FileFormat::kCollision);

  if (shadow_collision::IsOfType(f)) types.push_back(FileFormat::kShadowCollision);

  if (action::IsOfType(f)) types.push_back(FileFormat::kActionConfig);

  if (visanimation::IsOfType(f)) types.push_back(FileFormat::kVisanimationContainer);

  if (IsPlainText_(f)) types.push_back(FileFormat::kPlainText);

  if (minimap::IsOfType(f)) types.push_back(FileFormat::kMinimapConfig);

  return types;
}

FileFormat Detect(Reader& f) { return Detect_(f); }

FileFormat Detect(BufferView buffer) { return Detect_(buffer); }

FileFormat Detect(const std::filesystem::path& path) {
  FileReader f{path};
  return Detect_(f);
}

std::vector<FileFormat> DetectAll(Reader& f) { return DetectAll_(f); }

std::vector<FileFormat> DetectAll(BufferView f) { return DetectAll_(f); }

std::vector<FileFormat> DetectAll(const std::filesystem::path& path) {
  FileReader f{path};
  return DetectAll_(f);
}
}  // namespace format

}  // namespace nnl
