#include "NNL/game_asset/container/dig_entry.hpp"

#include "NNL/utility/data.hpp"
namespace nnl {

namespace dig_entry {

bool IsOfType_(Reader& f) {
  using namespace raw;

  f.Seek(0);

  const std::size_t data_size = f.Len();

  if (data_size < sizeof(RFileRecord)) return false;

  auto first_rec = f.ReadLE<RFileRecord>();

  if (first_rec.id != 0 || (std::size_t)first_rec.offset + first_rec.size > data_size ||
      first_rec.offset % sizeof(RFileRecord) != 0)
    return false;

  u32 num_records = first_rec.offset / sizeof(RFileRecord);

  if (num_records == 0) return false;

  f.Seek(0);

  if (data_size <= num_records * sizeof(RFileRecord)) return false;

  for (std::size_t i = 0; i < num_records; i++) {
    auto rec = f.ReadLE<RFileRecord>();
    if (rec.id != i || rec.reserved != 0 || data_size < rec.size + rec.offset ||
        rec.offset < num_records * sizeof(RFileRecord))
      return false;
  }

  return true;
}

bool IsOfType(Reader& f) { return IsOfType_(f); }

bool IsOfType(BufferView buffer) { return IsOfType_(buffer); }

bool IsOfType(const std::filesystem::path& path) {
  FileReader f{path};
  return IsOfType_(f);
}

DigEntry Import_(Reader& f) { return raw::Convert(raw::Parse(f)); }

DigEntry Import(Reader& f) { return Import_(f); }

DigEntry Import(BufferView buffer) { return Import_(buffer); }

DigEntry Import(const std::filesystem::path& path) {
  FileReader f{path};

  return Import_(f);
}

DigEntryView ImportView(BufferView buffer) { return raw::Convert(raw::ParseView(buffer)); }

template <typename CFCEntry>
void Export_(const CFCEntry& cfc_entry, Writer& f) {
  using namespace raw;

  NNL_EXPECTS(!cfc_entry.empty());

  f.Seek(0);

  auto file_records_off = f.MakeOffsetLE<RFileRecord>();

  for (std::size_t i = 0; i < cfc_entry.size(); i++) {
    auto& file_buffer = cfc_entry.at(i);

    NNL_EXPECTS(!file_buffer.empty());

    RFileRecord rfile_rec;
    rfile_rec.id = utl::data::narrow<u32>(i);
    rfile_rec.size = utl::data::narrow<u32>(file_buffer.size());
    f.WriteLE(rfile_rec);
  }

  for (std::size_t i = 0; i < cfc_entry.size(); i++) {
    auto& file = cfc_entry.at(i);
    file_records_off->*& RFileRecord::offset = utl::data::narrow<u32>(f.Tell());
    f.WriteBuf(file.data(), file.size());
    f.AlignData(0x10);
    ++file_records_off;
  }
}

Buffer Export(const DigEntry& cfc_entry) {
  BufferRW f;
  Export_(cfc_entry, f);
  return f;
}

void Export(const DigEntry& cfc_entry, Writer& f) { Export_(cfc_entry, f); }

void Export(const DigEntry& cfc_entry, const std::filesystem::path& path) {
  FileRW f{path, true};

  Export_(cfc_entry, f);
}

Buffer Export(const DigEntryView& cfc_entry) {
  BufferRW f;
  Export_(cfc_entry, f);
  return f;
}

void Export(const DigEntryView& cfc_entry, Writer& f) { Export_(cfc_entry, f); }

void Export(const DigEntryView& cfc_entry, const std::filesystem::path& path) {
  FileRW f{path, true};

  Export_(cfc_entry, f);
}

namespace raw {

template <typename TData, typename Reader>
RDigEntry<TData> Parse_(Reader f) {
  static_assert(std::is_base_of_v<nnl::Reader, std::remove_reference_t<Reader>>);

  RDigEntry<TData> rcfc_entry;

  f.Seek(0x8);
  NNL_TRY {
    u32 first_file_off = f.template ReadLE<u32>();

    if (first_file_off < sizeof(RFileRecord) || first_file_off % 0x10 != 0) {
      NNL_THROW(ParseError("invalid first offset"));
    }

    u32 num_records = first_file_off / sizeof(RFileRecord);

    f.Seek(0);

    rcfc_entry.file_records = f.template ReadArrayLE<RFileRecord>(num_records);

    for (const RFileRecord& file_rec : rcfc_entry.file_records) {
      f.Seek(file_rec.offset);

      if constexpr (std::is_same_v<Reader, Reader&>) {
        static_assert(std::is_same_v<TData, Buffer>);
        Buffer buffer = f.template ReadArrayLE<u8>(file_rec.size);
        rcfc_entry.file_buffers.insert({(u32)file_rec.offset, buffer});
      } else {
        rcfc_entry.file_buffers.insert({(u32)file_rec.offset, f.SubView(file_rec.offset, file_rec.size)});
      }
    }
  }
  NNL_CATCH(std::exception) { NNL_THROW(ParseError{NNL_ERMSG(e.what())}); }
  return rcfc_entry;
}

RDigEntry<Buffer> Parse(Reader& f) { return Parse_<Buffer, Reader&>(f); }

RDigEntry<BufferView> ParseView(BufferView buffer) { return Parse_<BufferView, BufferView>(buffer); }

template <typename TData>
std::vector<TData> Convert_(RDigEntry<TData>&& rcfc_entry) {
  std::vector<TData> cfc_entry;

  cfc_entry.reserve(rcfc_entry.file_records.size());

  for (auto& rfile_record : rcfc_entry.file_records) {
    if constexpr (std::is_same_v<TData, Buffer>) {
      cfc_entry.push_back(std::move(rcfc_entry.file_buffers.at(rfile_record.offset)));

    } else {
      cfc_entry.push_back(rcfc_entry.file_buffers.at(rfile_record.offset));
    }
  }
  rcfc_entry = {};
  return cfc_entry;
}

DigEntry Convert(RDigEntry<Buffer>&& rcfc_entry) { return Convert_(std::move(rcfc_entry)); }

DigEntryView Convert(RDigEntry<BufferView>&& rcfc_entry) { return Convert_(std::move(rcfc_entry)); }

}  // namespace raw

}  // namespace dig_entry

}  // namespace nnl
