/**
 * @file saudio.hpp
 * @brief Provides data structures for representing essential components of
 * audio data.
 *
 * This file defines the essential components of audio data. These structures
 * facilitate the conversion of audio data between various common
 * formats (such as WAV or MP3) and in-game formats.
 *
 * @see nnl::SAudio
 */
#pragma once

#include <filesystem>

#include "NNL/common/fixed_type.hpp"
#include "NNL/common/io.hpp"

namespace nnl {
/**
 * \defgroup SAudio Simple Audio
 * @ingroup SimpleAssets
 * @copydoc saudio.hpp
 * @{
 */
/**
 * @brief Represents a simple audio asset.
 *
 * This structure serves as intermediary representation of audio data. It
 * facilitate the conversion of audio data between various common formats (such
 * as WAV or MP3) and in-game formats.
 *
 * @see nnl::adpcm::Encode
 * @see nnl::adpcm::Decode
 * @see nnl::SAudio::Import
 * @see nnl::SAudio::ExportWAV
 */
struct SAudio {
  std::string name;  ///< An optional name of the audio asset

  u32 sample_rate = 0;  ///< The sample rate of the audio (per second)

  u16 num_channels = 1;  ///< The number of audio channels (1 for mono, 2 for stereo).

  std::vector<i16> pcm;  ///< The PCM audio data stored as 16-bit samples.

  /**
   * @brief Constructs an audio asset from a WAV file located at the specified file path.
   *
   * This method loads audio data from a WAV file located at the given path.
   *
   * @param path The file path to the WAV file.
   */
  [[nodiscard]] static SAudio Import(const std::filesystem::path& path);

  /**
   * @brief Constructs an audio asset from a WAV file in the buffer.
   *
   * This method loads audio data from a WAV file in the buffer.
   *
   * @param buffer The buffer containing a WAV file.
   */
  [[nodiscard]] static SAudio Import(BufferView buffer);

  /**
   * @brief Exports the audio data to a WAV file.
   *
   * This method saves the audio asset to the given file path in the WAV format.
   *
   * @param path The file path where the audio asset will be exported.
   */
  void ExportWAV(const std::filesystem::path& path) const;

  /**
   * @brief Exports the audio data to a WAV file.
   *
   * This method saves the audio asset in the WAV format to a buffer.
   */
  [[nodiscard]] Buffer ExportWAV() const;

  /**
   * @brief Converts stereo audio to mono.
   *
   * This method combines 2 channels into 1.
   */
  void ToMono();
};
/** @} */
}  // namespace nnl
