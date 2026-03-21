#include "NNL/simple_asset/saudio.hpp"

#include <gtest/gtest.h>

#include "NNL/game_asset/audio/adpcm.hpp"
#include "test_common.hpp"

using namespace nnl;

static std::vector<u8> wav_buffer;

TEST(SAudio, Export) {
  FileReader f{GetPath("naruto_adpcm")};
  f.Seek(0);

  auto adpcm_buff = f.ReadArrayLE<u8>(0x1600);

  SAudio wav;

  wav.num_channels = 1;
  wav.sample_rate = 15000;
  wav.pcm = adpcm::Decode(adpcm_buff);

  wav_buffer = wav.ExportWAV();

  ASSERT_EQ(wav_buffer.size(), 19700);
}

TEST(SAudio, Import) {
  ASSERT_TRUE(!wav_buffer.empty());

  SAudio wav = SAudio::Import(wav_buffer);

  ASSERT_EQ(wav.num_channels, 1);

  ASSERT_EQ(wav.sample_rate, 15000);

  ASSERT_EQ(wav.pcm.size(), 9828);
}
