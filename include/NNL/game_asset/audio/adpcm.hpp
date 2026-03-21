/**
 * @file adpcm.hpp
 * @brief Contains functions to encode and decode ADPCM audio buffers.
 *
 * ADPCM compression is used in PHD/PBD sound banks.
 *
 * @see nnl::adpcm::Encode
 * @see nnl::adpcm::Decode
 * @see nnl::asset::SoundBank
 * @see nnl::phd::IsOfType
 */
#pragma once

#include "NNL/common/fixed_type.hpp"
#include "NNL/common/io.hpp"

namespace nnl {
/**
 * \defgroup ADPCM ADPCM
 * @ingroup Audio
 * @copydoc adpcm.hpp
 * @{
 */

/**
 * @copydoc adpcm.hpp
 *
 */
namespace adpcm {
/**
 * \defgroup ADPCM_Main Main
 * @ingroup ADPCM
 * @{
 */
/**
 * @brief Encodes a PCM audio buffer into ADPCM format.
 *
 * This function takes a vector of PCM audio samples and encodes it into an
 * ADPCM buffer.
 *
 * @param pcm A vector of 16-bit signed integers representing the PCM audio
 * samples.
 * @return Buffer The encoded ADPCM audio buffer.
 *
 * @see nnl::adpcm::Decode
 */
Buffer Encode(const std::vector<i16>& pcm);
/**
 * @brief Decodes an ADPCM audio buffer into PCM format.
 *
 * This function takes an ADPCM encoded buffer and decodes it back into a
 * vector of PCM audio samples.
 *
 * @param adpcm A buffer representing the ADPCM encoded audio data.
 * @return A vector of 16-bit PCM audio samples.
 *
 * @see nnl::adpcm::Encode
 */
std::vector<i16> Decode(BufferView adpcm);
/** @} */
}  // namespace adpcm

/** @} */
}  // namespace nnl
