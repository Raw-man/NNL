
#include "NNL/game_asset/container/asset.hpp"

#include "NNL/game_asset/format.hpp"
#include "NNL/utility/data.hpp"
#include "NNL/utility/math.hpp"
namespace nnl {

namespace asset {

constexpr std::size_t kMaxEntryNum_ = 0xFF;  // Typically, it doesn't exceed 0xA

bool IsOfType_(Reader& f) {
  using namespace raw;

  f.Seek(0);

  const std::size_t data_size = f.Len();

  if (data_size < sizeof(u32)) return false;

  const auto num_records = f.ReadLE<u32>();

  // Empty containers should be allowed
  if (num_records > kMaxEntryNum_) return false;

  std::size_t header_size = sizeof(u32) + num_records * sizeof(RFileRecord);

  std::size_t padded_header_size = utl::math::RoundNum(header_size, 0x10);

  if (data_size < padded_header_size) return false;

  bool has_entries = false;

  for (std::size_t i = 0; i < num_records; i++) {
    auto record = f.ReadLE<RFileRecord>();

    if (record.offset == 0 && record.size != 0) return false;

    if (record.offset > 0 && record.offset < padded_header_size) return false;

    if (data_size < (std::size_t)record.offset + record.size) return false;

    if (record.offset % 0x10 != 0) return false;

    has_entries |= (record.offset > 0 && record.size > 0);
  }
  // Containers where all entries are null exist, check for the exact
  // size to avoid false positives.
  if (!has_entries && data_size != padded_header_size) return false;

  return true;
}

bool IsOfType(Reader& f) { return IsOfType_(f); }

bool IsOfType(BufferView buffer) { return IsOfType_(buffer); }

bool IsOfType(const std::filesystem::path& path) {
  FileReader f{path};
  return IsOfType_(f);
}

Asset Import_(Reader& f) { return raw::Convert(raw::Parse(f)); }

Asset Import(Reader& f) { return Import_(f); }

Asset Import(BufferView buffer) { return Import_(buffer); }

Asset Import(const std::filesystem::path& path) {
  FileReader f{path};

  return Import_(f);
}

AssetView ImportView(BufferView buffer) { return raw::Convert(raw::ParseView(buffer)); }

template <typename Asset>
void Export_(const Asset& asset_container, Writer& f) {
  using namespace raw;
  f.Seek(0);

  NNL_EXPECTS(asset_container.size() <= kMaxEntryNum_);  // should be plenty

  u32 max_num_records = asset_container.empty()
                            ? 0
                            : utl::data::narrow_cast<u32>(1U + std::prev(asset_container.end())->first);  // last key+1

  f.WriteLE<u32>(max_num_records);

  auto entry_start = f.MakeOffsetLE<RFileRecord>();

  for (u32 i = 0; i < max_num_records; i++) {
    RFileRecord entry;
    f.WriteLE(entry);
  }

  f.AlignData(0x10 /*, 0x30*/);  // sometimes padded with '0'

  for (u32 i = 0; i < max_num_records; i++) {
    auto itr_file_rec = asset_container.find(i);

    if (itr_file_rec != asset_container.end()) {
      auto& file_buffer = itr_file_rec->second;

      entry_start->*& RFileRecord::offset = utl::data::narrow<u32>(f.Tell());
      entry_start->*& RFileRecord::size = utl::data::narrow<u32>(file_buffer.size());
      f.WriteBuf(file_buffer.data(), file_buffer.size());
      f.AlignData(0x10);
    }
    ++entry_start;
  }
}

Buffer Export(const Asset& asset_archive) {
  BufferRW f;
  Export_(asset_archive, f);
  return f;
}

void Export(const Asset& asset_archive, Writer& f) { Export_(asset_archive, f); }

void Export(const Asset& asset_archive, const std::filesystem::path& path) {
  FileRW f{path, true};
  Export_(asset_archive, f);
}

Buffer Export(const AssetView& asset_archive) {
  BufferRW f;
  Export_(asset_archive, f);
  return f;
}

void Export(const AssetView& asset_archive, Writer& f) { Export_(asset_archive, f); }

void Export(const AssetView& asset_archive, const std::filesystem::path& path) {
  FileRW f{path, true};
  Export_(asset_archive, f);
}

template <typename Asset>
Category Categorize_(const Asset& asset_container) {
  std::map<u32, format::FileFormat> types;

  for (auto& [key, buffer] : asset_container) {
    types.insert({key, format::Detect(buffer)});
  }

  if (asset_container.empty()) return Category::kPlaceholder;

  if (asset_container.count(Asset3D::kModel) && types.at(Asset3D::kModel) == format::kModel) {
    return Category::kAsset3DModel;
  }

  if ((asset_container.count(Asset3D::kAnimationContainer) &&
       types.at(Asset3D::kAnimationContainer) == format::kAnimationContainer) ||
      (asset_container.count(Asset3D::kVisanimationContainer) &&
       types.at(Asset3D::kVisanimationContainer) == format::kVisanimationContainer)) {
    return Category::kAsset3DAnim;
  }

  if ((asset_container.count(Asset3D::kCollectionSpline) &&
       types.at(Asset3D::kCollectionSpline) == format::kCollection) &&
      (asset_container.count(Asset3D::kTextureContainer2) &&
       types.at(Asset3D::kTextureContainer2) == format::kTextureContainer)) {
    return Category::kAsset3DEffect;
  }

  if (asset_container.count(Asset3D::kActionConfig) && types.at(Asset3D::kActionConfig) == format::kActionConfig) {
    return Category::kAsset3DAction;
  }

  if (asset_container.count(SoundBank::kPHD) && types.at(SoundBank::kPHD) == format::kPHD &&
      asset_container.size() == 2) {
    return Category::kSoundBank;
  }

  if ((asset_container.count(BitmapText::kTextureContainer) &&
       types.at(BitmapText::kTextureContainer) == format::kTextureContainer) &&
      asset_container.count(BitmapText::kAdvanceWidth) &&
      (asset_container.count(BitmapText::kText) && types.at(BitmapText::kText) == format::kText) &&
      asset_container.size() == 3) {
    return Category::kBitmapTextFull;
  }

  if ((asset_container.count(BitmapText::kTextureContainer) &&
       types.at(BitmapText::kTextureContainer) == format::kTextureContainer) &&
      (asset_container.count(BitmapText::kAdvanceWidth) &&
       asset_container.at(BitmapText::kAdvanceWidth).size() <= 1024) &&
      asset_container.size() == 2) {
    auto& space = asset_container.at(1);

    bool all_positive = std::all_of(space.begin(), space.end(), [](auto n) { return n != 0; });

    if (all_positive) return Category::kBitmapTextFont;
  }

  if (std::all_of(types.begin(), types.end(), [](const auto id_type) { return id_type.second == format::kUIConfig; })) {
    return Category::kUIConfigs;
  }

  if (!types.empty() && types.size() % 2 == 0) {
    bool ui_conf_texture_pairs = true;
    for (auto it = types.begin(); it != types.end();) {
      auto id_type_0 = *it;
      it++;
      auto id_type_1 = *it;

      if (id_type_0.second != format::kUIConfig || id_type_1.second != format::kTextureContainer) {
        ui_conf_texture_pairs = false;
        break;
      }

      it++;
    }

    if (ui_conf_texture_pairs) return Category::kUIConfigTextureContainer;
  }

  return Category::kUnknown;
}

Category Categorize(const Asset& asset_container) { return Categorize_(asset_container); }

Category Categorize(const AssetView& asset_container) { return Categorize_(asset_container); }

namespace raw {

template <typename TData, typename Reader>
RAsset<TData> Parse_(Reader f) {
  static_assert(std::is_base_of_v<nnl::Reader, std::remove_reference_t<Reader>>, "");
  using namespace raw;

  f.Seek(0);

  RAsset<TData> asset_archive;

  NNL_TRY {
    u32 max_num_records = f.template ReadLE<u32>();

    if (max_num_records > kMaxEntryNum_) {
      NNL_THROW(ParseError(NNL_SRCTAG("too many elements, not an asset container (?)")));
    }

    asset_archive.file_records = f.template ReadArrayLE<RFileRecord>(max_num_records);

    for (const RFileRecord& file_record : asset_archive.file_records) {
      if (file_record.offset == 0) continue;

      f.Seek(file_record.offset);

      if constexpr (std::is_same_v<Reader, Reader&>) {
        static_assert(std::is_same_v<TData, Buffer>);
        Buffer b = f.template ReadArrayLE<u8>(file_record.size);

        asset_archive.file_buffers.insert({file_record.offset, std::move(b)});
      } else {
        asset_archive.file_buffers.insert({file_record.offset, f.SubView(file_record.offset, file_record.size)});
      }
    }
  }
  NNL_CATCH(std::exception) { NNL_THROW(ParseError{NNL_SRCTAG(e.what())}); }

  return asset_archive;
}

RAsset<Buffer> Parse(Reader& f) { return Parse_<Buffer, Reader&>(f); }

RAsset<BufferView> ParseView(BufferView buffer) { return Parse_<BufferView, BufferView>(buffer); }

template <typename TData>
std::map<u32, TData> Convert_(RAsset<TData>&& rasset) {
  std::map<u32, TData> asset;

  for (std::size_t i = 0; i < rasset.file_records.size(); i++) {
    auto file_record = rasset.file_records.at(i);
    if (file_record.offset == 0) continue;

    auto [itr_beg, itr_end] = rasset.file_buffers.equal_range(file_record.offset);

    auto itr_buf =
        std::find_if(itr_beg, itr_end, [&file_record](auto& el) { return el.second.size() == file_record.size; });

    assert(itr_beg != itr_end && itr_buf != itr_end);

    auto& buffer = itr_buf->second;

    if constexpr (std::is_same_v<TData, Buffer>) {
      asset.insert({(u32)i, std::move(buffer)});
    } else {
      asset.insert({(u32)i, buffer});
    }
  }

  rasset = {};

  return asset;
}

Asset Convert(RAsset<Buffer>&& rasset) { return Convert_(std::move(rasset)); }

AssetView Convert(RAsset<BufferView>&& rasset) { return Convert_(std::move(rasset)); }

}  // namespace raw

}  // namespace asset

}  // namespace nnl
