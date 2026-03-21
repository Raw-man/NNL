#include "NNL/game_asset/container/dig.hpp"

namespace nnl {

namespace dig {

Buffer Decompress(BufferView buffer, u32 decompressed_size) {
  u32 index = 0;         // Position in the input buffer
  u32 dest_index = 0;    // Destination of a decoded byte in the output buffer
  u8 last_dec_byte = 0;  // Last decoded byte of the previous decoding iteration
  u8 bit_shift = 0;      // A right shift value

  std::vector<u8> frequencies(256, 0);
  // References to previously decoded sequences - sliding window,
  // 2d array [256][32]
  std::vector<u32> seq_indices(8192, 0);
  Buffer decomp_buffer(decompressed_size, 0);

  while (index + 1 < buffer.size() && dest_index < decomp_buffer.size()) {
    u16 next_code = buffer[index + 1];  // next pair of bytes that form a code
    next_code = next_code << 8;
    next_code = next_code | buffer[index];
    next_code = next_code >> bit_shift;  // unfold a 9 bit code

    // The result can be interpreted as follows:
    //  iiiiiiif|ooooolll //f=0
    //  iiiiiiif|bbbbbbbb //f=1
    // i - ignore
    // f - flag  (is literal or offset/length pair)
    // l - length (add 1 to get the real length)
    // o - occurrences/frequency
    // b - byte literal

    ++bit_shift;
    ++index;

    if (bit_shift == 8) {
      bit_shift = 0;
      ++index;
    }

    u32 seq_index = dest_index;  // start of a byte sequence

    // Is next_code a literal or a reference?
    if ((next_code & 0x100) != 0) {
      decomp_buffer[dest_index] = next_code & 0xFF;  // Store the literal
      ++dest_index;

    } else {
      // 0x1F + 0xFF*32 = 8191
      u16 key = ((next_code >> 3) & 0x1F) + last_dec_byte * 32;

      // Get a reference to a previously decoded sequence
      u32 src_index = seq_indices[key];

      for (u8 length = 0;
           length < (next_code & 0x07) + 1 && src_index < decomp_buffer.size() && dest_index < decomp_buffer.size();
           ++length, ++dest_index, ++src_index) {
        // Copy a previously decoded  sequence (up to 8 bytes)
        decomp_buffer[dest_index] = decomp_buffer[src_index];
      }
    }

    if (dest_index >= decomp_buffer.size()) break;

    // 0x1F + 0xFF*32 = 8191
    u16 key = frequencies[last_dec_byte] + last_dec_byte * 32;

    seq_indices[key] = seq_index;

    frequencies[last_dec_byte] = (frequencies[last_dec_byte] + 1) & 0x1F;  // increase by 1 (up to 31)

    last_dec_byte = decomp_buffer[dest_index - 1];
  }

  return decomp_buffer;
}

Buffer Compress(BufferView buffer) {
  u32 index = 0;

  u8 last_enc_byte = 0;

  u8 bit_shift = 0;

  std::vector<u32> frequencies(256, 0);

  std::vector<u32> seq_indices(8192, 0);

  std::vector<u16> codes;

  Buffer compressed_buffer;

  codes.reserve(buffer.Len());

  compressed_buffer.reserve(buffer.Len());

  while (index < buffer.Len()) {
    u8 best_freq = 0;

    u8 best_match = 0;

    u8 positions_to_check = frequencies[last_enc_byte] < 32 ? (frequencies[last_enc_byte] & 0x1F) : 32;

    u32 seq_index = index;

    for (u8 freq = 0; freq < positions_to_check; freq++) {
      u16 key = freq + last_enc_byte * 32;  // 0x1F + 0xFF*32 = 8191

      u32 src_index = seq_indices[key];

      u8 matched = 0;

      u8 max_length = index + 8 < buffer.Len() ? 8 : buffer.Len() - index;

      for (u8 offset = 0; offset < max_length; ++offset) {
        if (buffer[src_index + offset] == buffer[index + offset])
          ++matched;
        else
          break;
      }

      if (matched > best_match) {
        best_freq = freq;
        best_match = matched;
      }
    }

    u16 code = 0;

    if (best_match > 0)  // found a better match?
    {
      code = code | (best_freq << 3);
      code = code | (best_match - 1);  // Encode a reference
      index += best_match;
    } else {
      // Encode a byte literal
      code = 0x100 | buffer[index];  // f|bbbbbbbb //f=1
      ++index;
    }

    code = code << bit_shift;  // Prepare for folding

    codes.push_back(code);

    ++bit_shift;

    if (bit_shift == 8) bit_shift = 0;

    u16 key = (frequencies[last_enc_byte] & 0x1F) + last_enc_byte * 32;  // 0x1F + 0xFF*32 = 8191

    seq_indices[key] = seq_index;

    frequencies[last_enc_byte] = frequencies[last_enc_byte] + 1;  // Increase by 1 (up to 31)

    last_enc_byte = buffer[index - 1];
  }

  // Fold codes (8 codes, 16 bytes -> 8 codes, 9 bytes)
  for (u32 i = 0; i < codes.size(); i = i + 8) {
    u8 group_size = i + 8 < codes.size() ? 8 : codes.size() - i;

    for (u8 s = 0; s <= group_size; s += 2) {
      u16 first = s > 0 ? codes[s + i - 1] : 0x00;
      u16 middle = s < group_size ? codes[s + i] : 0x00;
      u16 last = s < group_size - 1 ? codes[s + i + 1] : 0x00;

      u16 result = middle | (first >> 8) | (last << 8);

      compressed_buffer.push_back(result & 0xFF);

      if (s < group_size) compressed_buffer.push_back(result >> 8);
    }
  }

  return compressed_buffer;
}
}  // namespace dig
}  // namespace nnl
