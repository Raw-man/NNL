#include "NNL/game_asset/audio/phd.hpp"

namespace nnl {
namespace phd {

bool IsOfType_(Reader& f) {
  using namespace raw;

  f.Seek(0);

  if (f.Len() < sizeof(RCommonAttr)) return false;

  auto magic_bytes = f.ReadLE<u32>();

  if (magic_bytes != kMagicBytes) return false;

  return true;
}

bool IsOfType(Reader& f) { return IsOfType_(f); }

bool IsOfType(BufferView buffer) { return IsOfType_(buffer); }

bool IsOfType(const std::filesystem::path& path) {
  FileReader f{path};
  return IsOfType_(f);
}

}  // namespace phd
}  // namespace nnl
