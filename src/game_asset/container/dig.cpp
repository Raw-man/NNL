#include "NNL/game_asset/container/dig.hpp"

#include "NNL/utility/data.hpp"

namespace nnl {

namespace dig {

constexpr u32 kMaxBlockOffset = std::numeric_limits<u32>::max() / raw::kBlockSize;  // Should be plenty

bool IsValid_(const raw::RFileRecord& file_rec) {
  using namespace raw;
  if (file_rec.is_compressed > 1) return false;

  if (file_rec.offset > kMaxBlockOffset) return false;

  if (file_rec.offset != 0 && (file_rec.compressed_size == 0 || file_rec.decompressed_size == 0 ||
                               file_rec.num_entries == 0 || file_rec.compressed_size > file_rec.decompressed_size))
    return false;

  if (!file_rec.is_compressed && file_rec.compressed_size != file_rec.decompressed_size) return false;

  if (file_rec.is_compressed &&
      (file_rec.offset == 0 || file_rec.compressed_size == 0 || file_rec.decompressed_size == 0))
    return false;

  return true;
}
// The format does not store a concrete number of entries.
// In the Japanese demo of NSUNI, many entries are empty placeholders
u32 EstimateNumRecords_(Reader& f) {
  using namespace raw;
  f.Seek(0);

  RFileRecord file_rec;

  while (f.Tell() + sizeof(RFileRecord) <= f.Len()) {
    file_rec = f.ReadLE<RFileRecord>();
    if (!IsValid_(file_rec)) return 0;
    if (file_rec.offset != 0) break;
  }

  return (file_rec.offset * kBlockSize) / sizeof(RFileRecord);
}

bool IsOfType_(Reader& f) {
  using namespace raw;

  f.Seek(0);

  const std::size_t data_size = f.Len();

  if (data_size <= kBlockSize) return false;

  u32 num_records = EstimateNumRecords_(f);

  if (num_records == 0) return false;

  f.Seek(0);

  if (data_size <= num_records * sizeof(RFileRecord)) return false;

  for (std::size_t i = 0; i < num_records; i++) {
    auto file_rec = f.ReadLE<RFileRecord>();
    if (!IsValid_(file_rec)) return false;

    if ((std::size_t)file_rec.offset * kBlockSize + file_rec.compressed_size > data_size) return false;
  }

  return true;
}

bool IsOfType(Reader& f) { return IsOfType_(f); }

bool IsOfType(BufferView buffer) { return IsOfType_(buffer); }

bool IsOfType(const std::filesystem::path& path) {
  FileReader f{path};
  return IsOfType_(f);
}

Dig Import_(Reader& f) { return raw::Convert(raw::Parse(f)); }

Dig Import(Reader& f) { return Import_(f); }

Dig Import(BufferView buffer) { return Import_(buffer); }

Dig Import(const std::filesystem::path& path) {
  FileReader f{path};

  return Import_(f);
}

DigView ImportView(BufferView buffer) { return raw::Convert(raw::ParseView(buffer)); }

template <typename CFC = Dig>
void Export_(const CFC& cfcdig, Writer& f) {
  using namespace raw;

  NNL_EXPECTS(!cfcdig.empty());

  f.Seek(0);

  auto file_rec_off = f.MakeOffsetLE<RFileRecord>();

  bool has_non_empty_entries = false;

  for (auto& file_record : cfcdig) {
    NNL_EXPECTS(!file_record.is_compressed || !file_record.buffer.empty());
    NNL_EXPECTS(!file_record.is_compressed || file_record.decompressed_size > file_record.buffer.size());
    NNL_EXPECTS(file_record.buffer.empty() || file_record.num_entries != 0);
    has_non_empty_entries |= !file_record.buffer.empty();
    RFileRecord rfile_record;

    if (!file_record.buffer.empty()) {
      rfile_record.num_entries = file_record.num_entries;
      rfile_record.compressed_size = utl::data::narrow<u32>(file_record.buffer.size());
      rfile_record.decompressed_size =
          file_record.is_compressed ? file_record.decompressed_size : file_record.buffer.size();
      rfile_record.is_compressed = file_record.is_compressed;
    }

    f.WriteLE(rfile_record);
  }

  NNL_EXPECTS(has_non_empty_entries);

  for (auto& file_record : cfcdig) {
    if (!file_record.buffer.empty()) {
      f.AlignData(kBlockSize);
      file_rec_off->*& RFileRecord::offset = f.Tell() / kBlockSize;
      f.WriteBuf(file_record.buffer.data(), file_record.buffer.size());
    }
    ++file_rec_off;
  }

  f.AlignData(kBlockSize);
}

Buffer Export(const Dig& cfcdig) {
  BufferRW f;
  Export_(cfcdig, f);
  return f;
}

void Export(const Dig& cfcdig, Writer& f) { Export_(cfcdig, f); }

void Export(const Dig& cfcdig, const std::filesystem::path& path) {
  FileRW f{path, true};
  Export_(cfcdig, f);
}

Buffer Export(const DigView& cfcdig) {
  BufferRW f;
  Export_(cfcdig, f);
  return f;
}

void Export(const DigView& cfcdig, Writer& f) { Export_(cfcdig, f); }

void Export(const DigView& cfcdig, const std::filesystem::path& path) {
  FileRW f{path, true};
  Export_(cfcdig, f);
}

namespace raw {

template <typename TData, typename Reader>
RDig<TData> Parse_(Reader f) {
  static_assert(std::is_base_of_v<nnl::Reader, std::remove_reference_t<Reader>>);
  RDig<TData> rcfc_dig;
  f.Seek(0);

  u32 num_records = EstimateNumRecords_(f);

  if (num_records == 0) {
    NNL_THROW(ParseError(NNL_SRCTAG("invalid num records")));
  }

  f.Seek(0);

  NNL_TRY {
    for (std::size_t i = 0; i < num_records; i++) {
      auto& file_rec = rcfc_dig.file_records.emplace_back(f.template ReadLE<RFileRecord>());

      if (!IsValid_(file_rec)) {
        rcfc_dig.file_records.pop_back();
        break;
      }

      auto next_rec_offset = f.Tell();

      f.Seek(file_rec.offset * kBlockSize);

      if constexpr (std::is_same_v<Reader, Reader&>) {
        static_assert(std::is_same_v<TData, Buffer>);
        Buffer buffer = f.template ReadArrayLE<u8>(file_rec.compressed_size);
        rcfc_dig.file_buffers.insert({(u32)file_rec.offset, std::move(buffer)});
      } else {
        rcfc_dig.file_buffers.insert({(u32)file_rec.offset, f.SubView(f.Tell(), file_rec.compressed_size)});
      }

      f.Seek(next_rec_offset);
    }
  }
  NNL_CATCH(std::exception) { NNL_THROW(ParseError{NNL_SRCTAG(e.what())}); }

  return rcfc_dig;
}

RDig<Buffer> Parse(Reader& f) { return Parse_<Buffer, Reader&>(f); }

RDig<BufferView> ParseView(BufferView buffer) { return Parse_<BufferView, BufferView>(buffer); }

template <typename TData>
std::vector<TFileRecord<TData>> Convert_(RDig<TData>&& rcfc_dig) {
  std::vector<TFileRecord<TData>> cfc;
  cfc.reserve(rcfc_dig.file_records.size());

  for (auto& rfile_record : rcfc_dig.file_records) {
    auto& file_record = cfc.emplace_back();
    file_record.num_entries = rfile_record.num_entries;
    file_record.decompressed_size = rfile_record.decompressed_size;
    file_record.is_compressed = rfile_record.is_compressed;

    if constexpr (std::is_same_v<TData, Buffer>) {
      file_record.buffer = std::move(rcfc_dig.file_buffers.at(rfile_record.offset));
    } else {
      file_record.buffer = rcfc_dig.file_buffers.at(rfile_record.offset);
    }
  }

  return cfc;
}

Dig Convert(RDig<Buffer>&& cfcdig) { return Convert_(std::move(cfcdig)); }

DigView Convert(RDig<BufferView>&& cfcdig) { return Convert_(std::move(cfcdig)); }

}  // namespace raw

}  // namespace dig

}  // namespace nnl
