
#include "NNL/game_asset/container/collection.hpp"

#include "NNL/game_asset/container/asset.hpp"
#include "NNL/utility/data.hpp"
namespace nnl {

namespace collection {

bool IsOfType_(Reader& f) {
  using namespace raw;

  f.Seek(0);

  const std::size_t data_size = f.Len();

  if (data_size < sizeof(u32)) return false;

  u32 num_records = f.ReadLE<u32>();

  if (num_records == 0) return false;

  std::size_t header_size = sizeof(u32) + num_records * sizeof(RFileRecord);

  if (data_size < header_size) return false;

  bool offsets_increase = true;

  std::size_t prev_offset = 0;

  auto first_rec = f.ReadLE<RFileRecord>();

  f.Seek(sizeof(u32));

  for (std::size_t i = 0; i < num_records; i++) {
    auto file_rec = f.ReadLE<RFileRecord>();

    if (file_rec.offset < header_size || data_size <= file_rec.offset) return false;

    offsets_increase &= file_rec.offset > prev_offset;

    if (!offsets_increase) return false;

    std::size_t prev_size = file_rec.offset - prev_offset;

    if (prev_size < 0x4) return false;
  }

  std::size_t padding_size = first_rec.offset - header_size;

  auto padding = f.ReadArrayLE<u8>(padding_size);
  // All padding bytes should be the same
  for (std::size_t i = 1; i < padding.size(); i++) {
    if (padding[i] != padding[i - 1]) {
      return false;
    }
  }

  return true;
}

bool IsOfType(Reader& f) { return IsOfType_(f); }

bool IsOfType(BufferView buffer) { return IsOfType_(buffer); }

bool IsOfType(const std::filesystem::path& path) {
  FileReader f{path};
  return IsOfType_(f);
}

Collection Import_(Reader& f) { return raw::Convert(raw::Parse(f)); }

Collection Import(Reader& f) { return Import_(f); }

Collection Import(BufferView buffer) { return Import_(buffer); }

Collection Import(const std::filesystem::path& path) {
  FileReader f{path};

  return Import_(f);
}

CollectionView ImportView(BufferView buffer) { return raw::Convert(raw::ParseView(buffer)); }

template <typename Collection>
void Export_(const Collection& asset_collection, Writer& f) {
  using namespace raw;

  NNL_EXPECTS(!asset_collection.empty());

  f.Seek(0);

  f.WriteLE<u32>(utl::data::narrow<u32>(asset_collection.size()));

  auto file_records_off = f.MakeOffsetLE<RFileRecord>();

  for (std::size_t i = 0; i < asset_collection.size(); i++) {
    f.WriteLE<u32>(0);
  }

  for (auto& file_buffer : asset_collection) {
    if (asset::IsOfType(file_buffer)) f.AlignData(0x10, '0');

    f.AlignData(0x4);

    file_records_off->*& RFileRecord::offset = utl::data::narrow<u32>(f.Tell());
    f.WriteBuf(file_buffer.data(), file_buffer.size());
    ++file_records_off;
  }
}

Buffer Export(const Collection& asset_collection) {
  BufferRW f;
  Export_(asset_collection, f);
  return f;
}

void Export(const Collection& asset_collection, Writer& f) { Export_(asset_collection, f); }

void Export(const Collection& asset_collection, const std::filesystem::path& path) {
  FileRW f{path, true};

  Export_(asset_collection, f);
}

Buffer Export(const CollectionView& asset_collection) {
  BufferRW f;
  Export_(asset_collection, f);
  return f;
}

void Export(const CollectionView& asset_collection, Writer& f) { Export_(asset_collection, f); }

void Export(const CollectionView& asset_collection, const std::filesystem::path& path) {
  FileRW f{path, true};
  Export_(asset_collection, f);
}

namespace raw {

template <typename TData, typename Reader>
RCollection<TData> Parse_(Reader f) {
  static_assert(std::is_base_of_v<nnl::Reader, std::remove_reference_t<Reader>>);

  RCollection<TData> rs_col;

  f.Seek(0);

  NNL_TRY {
    u32 num_records = f.template ReadLE<u32>();

    if (num_records == 0) {
      NNL_THROW(ParseError(NNL_ERMSG("invalid num records")));
    }

    rs_col.file_records = f.template ReadArrayLE<RFileRecord>(num_records);

    std::size_t data_size = f.Len();

    for (std::size_t i = 0; i < num_records; i++) {
      auto rfile_rec = rs_col.file_records.at(i);

      std::size_t entry_size =
          i + 1 < num_records ? rs_col.file_records.at(i + 1).offset - rfile_rec.offset : data_size - rfile_rec.offset;

      f.Seek(rfile_rec.offset);

      if constexpr (std::is_same_v<Reader, Reader&>) {
        static_assert(std::is_same_v<TData, Buffer>);
        Buffer buffer = f.template ReadArrayLE<u8>(entry_size);
        rs_col.file_buffers.insert({(u32)rfile_rec.offset, buffer});
      } else {
        rs_col.file_buffers.insert({(u32)rfile_rec.offset, f.SubView(rfile_rec.offset, entry_size)});
      }
    }
  }
  NNL_CATCH(std::exception) { NNL_THROW(ParseError{NNL_ERMSG(e.what())}); }

  return rs_col;
}

RCollection<Buffer> Parse(Reader& f) { return Parse_<Buffer, Reader&>(f); }

RCollection<BufferView> ParseView(BufferView buffer) { return Parse_<BufferView, BufferView>(buffer); }

template <typename TData>
std::vector<TData> Convert_(RCollection<TData>&& rs_col) {
  std::vector<TData> s_col;

  s_col.reserve(rs_col.file_records.size());

  for (auto rfile_record : rs_col.file_records) {
    if constexpr (std::is_same_v<TData, Buffer>) {
      s_col.push_back(std::move(rs_col.file_buffers.at(rfile_record.offset)));

    } else {
      s_col.push_back(rs_col.file_buffers.at(rfile_record.offset));
    }
  }

  rs_col = {};

  return s_col;
}

Collection Convert(RCollection<Buffer>&& rasset_collection) { return Convert_(std::move(rasset_collection)); }

CollectionView Convert(RCollection<BufferView>&& rasset_collection) { return Convert_(std::move(rasset_collection)); }

}  // namespace raw

}  // namespace collection

}  // namespace nnl
