#include "NNL/game_asset/visual/ui.hpp"

namespace nnl {
namespace ui {

bool IsOfType_(Reader& f) {
  using namespace raw;
  f.Seek(0);
  const std::size_t data_size = f.Len();
  if (data_size < sizeof(RHeader)) return false;
  auto header = f.ReadLE<RHeader>();

  if (header.magic_bytes != kMagicBytes) return false;

  if (header.offset_struct_0 > data_size || header.offset_struct_0 < sizeof(RHeader)) return false;

  if (header.offset_struct_1 > data_size || header.offset_struct_1 < sizeof(RHeader)) return false;

  if (header.offset_struct_2 > data_size || header.offset_struct_2 < sizeof(RHeader)) return false;

  return true;
}

bool IsOfType(Reader& f) { return IsOfType_(f); }

bool IsOfType(BufferView buffer) { return IsOfType_(buffer); }

bool IsOfType(const std::filesystem::path& path) {
  FileReader f{path};
  return IsOfType_(f);
}
}  // namespace ui
}  // namespace nnl
