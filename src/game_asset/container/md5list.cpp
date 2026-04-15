#include "NNL/game_asset/container/md5list.hpp"

#include "NNL/utility/data.hpp"
namespace nnl {
namespace md5list {

template <typename CFCDig>
std::vector<std::array<u8, 16>> Generate_(const CFCDig& cfc_dig) {
  std::vector<std::array<u8, 16>> list(cfc_dig.size());
  for (std::size_t i = 0; i < list.size(); i++) {
    auto& file_rec = cfc_dig[i];
    list[i] = utl::data::MD5(file_rec.buffer);
  }

  return list;
}

std::vector<std::array<u8, 16>> Generate(const dig::Dig& cfc_dig) { return Generate_(cfc_dig); }

std::vector<std::array<u8, 16>> Generate(const dig::DigView& cfc_dig) { return Generate_(cfc_dig); }

std::vector<std::array<u8, 16>> Import_(Reader& f) {
  if (f.Len() % 0x10 != 0) NNL_THROW(ParseError{NNL_SRCTAG("invalid file size")});

  const std::size_t num_entries = f.Len() / 0x10;

  std::vector<std::array<u8, 16>> list(num_entries);

  f.Seek(0);

  for (std::size_t i = 0; i < num_entries; i++) {
    f.ReadBuf(list[i].data(), list[i].size());
  }
  return list;
}

std::vector<std::array<u8, 16>> Import(Reader& f) { return Import_(f); }

std::vector<std::array<u8, 16>> Import(BufferView buffer) { return Import_(buffer); }

std::vector<std::array<u8, 16>> Import(const std::filesystem::path& path) {
  FileReader f{path};
  return Import_(f);
}

void Export_(const std::vector<std::array<u8, 16>>& md5list, Writer& f) {
  f.Seek(0);

  NNL_EXPECTS(!md5list.empty());

  for (auto& md5sum : md5list) {
    f.WriteBuf(md5sum.data(), md5sum.size());
  }

  f.AlignData(dig::raw::kBlockSize);
}

void Export(const std::vector<std::array<u8, 16>>& md5list, Writer& f) { Export_(md5list, f); }

Buffer Export(const std::vector<std::array<u8, 16>>& md5list) {
  BufferRW f;
  Export_(md5list, f);
  return f;
}

void Export(const std::vector<std::array<u8, 16>>& md5list, const std::filesystem::path& path) {
  FileRW f{path, true};
  Export_(md5list, f);
}

}  // namespace md5list

}  // namespace nnl
