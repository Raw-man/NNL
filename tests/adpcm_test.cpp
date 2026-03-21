#include "NNL/game_asset/audio/adpcm.hpp"

#include <gtest/gtest.h>

#include "test_common.hpp"

using namespace nnl;

TEST(ADPCM, Decode) {
  FileReader f{GetPath("naruto_adpcm")};

  auto adpcm_buff = f.ReadArrayLE<u8>(f.Len());

  auto decoded_pcm = adpcm::Decode(adpcm_buff);

  ASSERT_EQ(decoded_pcm.size(), 9828);
}
