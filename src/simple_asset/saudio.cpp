
#include "NNL/simple_asset/saudio.hpp"

#include "NNL/utility/filesys.hpp"

namespace nnl {

NNL_PACK(struct ChunkInfo {
  char chunk_id[4];
  u32 chunk_size;
});

static_assert(sizeof(ChunkInfo) == 0x8, "");

NNL_PACK(struct RiffHeader {
  ChunkInfo chunk_info{{'R', 'I', 'F', 'F'}, 0};
  char wave[4] = {'W', 'A', 'V', 'E'};
});

static_assert(sizeof(RiffHeader) == 0xC, "");

NNL_PACK(struct FmtHeader {
  ChunkInfo chunk_info{{'f', 'm', 't', ' '}, 0x10};
  u16 audio_format = 1;
  u16 num_channels = 1;  // 1 - mono
  u32 sample_rate = 0;
  u32 byte_rate = 0;
  u16 block_align = 2;
  u16 bits_per_sample = 16;
});

static_assert(sizeof(FmtHeader) == (sizeof(ChunkInfo) + 0x10), "");

void Import_(SAudio& audio, BufferView buffer) {
  buffer.Seek(0);

  bool fmt_chunk_found = false;
  bool data_chunk_found = false;

  auto riff_header = buffer.ReadLE<RiffHeader>();
  if ((std::memcmp(riff_header.chunk_info.chunk_id, "RIFF", 4) != 0) ||
      (std::memcmp(riff_header.wave, "WAVE", 4) != 0)) {
    NNL_THROW(ParseError(NNL_ERMSG("not a wav file: " + audio.name)));
  }

  while (buffer.Tell() + sizeof(ChunkInfo) <= buffer.Len() && (!fmt_chunk_found || !data_chunk_found)) {
    auto cur_tell = buffer.Tell();

    auto chunk_info = buffer.ReadLE<ChunkInfo>();

    if ((std::memcmp(chunk_info.chunk_id, "fmt ", 4) == 0) && !fmt_chunk_found && chunk_info.chunk_size != 0) {
      fmt_chunk_found = true;
      buffer.Seek(cur_tell);
      auto fmt_header = buffer.ReadLE<FmtHeader>();

      if (fmt_header.num_channels == 0 || fmt_header.num_channels > 2) {
        NNL_THROW(ParseError(NNL_ERMSG("only mono/stereo audio is supported: " + audio.name)));
      }

      if (fmt_header.audio_format != 1 || fmt_header.bits_per_sample != 16) {
        NNL_THROW(ParseError(NNL_ERMSG("only 16 bit pcm audio is supported: " + audio.name)));
      }

      audio.num_channels = fmt_header.num_channels;

      audio.sample_rate = fmt_header.sample_rate;

    } else if ((std::memcmp(chunk_info.chunk_id, "data ", 4) == 0) && !data_chunk_found && chunk_info.chunk_size != 0) {
      data_chunk_found = true;
      audio.pcm = buffer.ReadArrayLE<i16>(chunk_info.chunk_size / 2);
    }
    // Go to the next chunk
    buffer.Seek(cur_tell + sizeof(ChunkInfo) + chunk_info.chunk_size);
  }

  if (!data_chunk_found || !fmt_chunk_found) {
    NNL_THROW(ParseError(NNL_ERMSG("no data found in the wav file: " + audio.name)));
  }
}

SAudio SAudio::Import(const std::filesystem::path& path) {
  SAudio saud;

  nnl::FileReader f{path};

  Buffer buf = f.ReadArrayLE<u8>(f.Len());

  f.Close();

  saud.name = utl::filesys::u8string(path.filename());

  Import_(saud, buf);

  return saud;
}

SAudio SAudio::Import(BufferView buffer) {
  SAudio saud;
  Import_(saud, buffer);
  return saud;
}

Buffer SAudio::ExportWAV() const {
  BufferRW f;
  NNL_EXPECTS(num_channels > 0);
  NNL_EXPECTS(sample_rate >= 4000);
  NNL_EXPECTS(!pcm.empty());
  NNL_EXPECTS(pcm.size() % num_channels == 0);
  auto riff_header = f.WriteLE(RiffHeader());

  FmtHeader fmt_header;
  fmt_header.num_channels = num_channels;
  fmt_header.sample_rate = sample_rate;
  fmt_header.byte_rate = sample_rate * sizeof(i16);

  f.WriteLE(fmt_header);

  ChunkInfo data_header;
  std::memcpy(&data_header.chunk_id, "data", 4);
  data_header.chunk_size = static_cast<u32>(pcm.size() * sizeof(i16));

  f.WriteLE(data_header);

  f.WriteArrayLE(pcm);

  auto chunk_info = riff_header->*&RiffHeader::chunk_info;
  chunk_info->*& ChunkInfo::chunk_size = f.Tell() - sizeof(ChunkInfo);

  return f;
}

void SAudio::ExportWAV(const std::filesystem::path& path) const {
  nnl::Buffer bin_wav = ExportWAV();
  nnl::FileRW f{path, true};
  f.WriteArrayLE(bin_wav);
}

void SAudio::ToMono() {
  NNL_EXPECTS(num_channels == 1 || num_channels == 2);
  if (num_channels == 2) {
    NNL_EXPECTS(pcm.size() % 2 == 0);
    std::vector<i16> new_buffer;
    new_buffer.reserve(pcm.size() / 2);
    for (std::size_t i = 1; i < pcm.size(); i += 2) {
      int left_channel = pcm[i - 1];
      int right_channel = pcm[i];
      i16 mixed_channel = static_cast<i16>((left_channel + right_channel) / 2);
      new_buffer.push_back(mixed_channel);
    }

    pcm = std::move(new_buffer);
    num_channels = 1;
  }
}

}  // namespace nnl
