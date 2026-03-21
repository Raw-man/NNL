#include "NNL/utility/data.hpp"

namespace nnl {

namespace utl::data {

u32 CRC32(BufferView data, u32 polynomial) noexcept {
  u32 table[256];

  for (u32 i = 0; i < 256; i++) {
    u32 c = i;
    for (size_t j = 0; j < 8; j++) {
      if (c & 1) {
        c = polynomial ^ (c >> 1);
      } else {
        c >>= 1;
      }
    }
    table[i] = c;
  }

  u32 c = 0 ^ 0xFFFFFFFF;
  const u8* u = static_cast<const u8*>(data.data());
  for (std::size_t i = 0; i < data.size(); ++i) {
    c = table[(c ^ u[i]) & 0xFF] ^ (c >> 8);
  }
  return c ^ 0xFFFFFFFF;
}

/*
 * The code is placed under public domain by Kazuho Oku <kazuhooku@gmail.com>.
 *
 * The MD5 implementation is based on a public domain implementation written by
 * Solar Designer <solar@openwall.com> in 2001, which is used by Dovecot.
 *
 */

#define NNL_MD5_F(x, y, z) ((z) ^ ((x) & ((y) ^ (z))))
#define NNL_MD5_G(x, y, z) ((y) ^ ((z) & ((x) ^ (y))))
#define NNL_MD5_H(x, y, z) ((x) ^ (y) ^ (z))
#define NNL_MD5_I(x, y, z) ((y) ^ ((x) | ~(z)))

#define NNL_MD5_STEP(f, a, b, c, d, x, t, s)                 \
  (a) += f((b), (c), (d)) + (x) + (t);                       \
  (a) = (((a) << (s)) | (((a) & 0xffffffff) >> (32 - (s)))); \
  (a) += (b);

#define NNL_MD5_SET(n)                                                                    \
  (this->block_[(n)] = (std::size_t)ptr[(n) * 4] | ((std::size_t)ptr[(n) * 4 + 1] << 8) | \
                       ((std::size_t)ptr[(n) * 4 + 2] << 16) | ((std::size_t)ptr[(n) * 4 + 3] << 24))
#define NNL_MD5_GET(n) (this->block_[(n)])

const unsigned char* MD5Context::Body(const unsigned char* data, std::size_t size) {
  const unsigned char* ptr = data;

  std::size_t a = this->a_;
  std::size_t b = this->b_;
  std::size_t c = this->c_;
  std::size_t d = this->d_;

  do {
    std::size_t saved_a = a;
    std::size_t saved_b = b;
    std::size_t saved_c = c;
    std::size_t saved_d = d;

    /* Round 1 */
    NNL_MD5_STEP(NNL_MD5_F, a, b, c, d, NNL_MD5_SET(0), 0xd76aa478, 7)
    NNL_MD5_STEP(NNL_MD5_F, d, a, b, c, NNL_MD5_SET(1), 0xe8c7b756, 12)
    NNL_MD5_STEP(NNL_MD5_F, c, d, a, b, NNL_MD5_SET(2), 0x242070db, 17)
    NNL_MD5_STEP(NNL_MD5_F, b, c, d, a, NNL_MD5_SET(3), 0xc1bdceee, 22)
    NNL_MD5_STEP(NNL_MD5_F, a, b, c, d, NNL_MD5_SET(4), 0xf57c0faf, 7)
    NNL_MD5_STEP(NNL_MD5_F, d, a, b, c, NNL_MD5_SET(5), 0x4787c62a, 12)
    NNL_MD5_STEP(NNL_MD5_F, c, d, a, b, NNL_MD5_SET(6), 0xa8304613, 17)
    NNL_MD5_STEP(NNL_MD5_F, b, c, d, a, NNL_MD5_SET(7), 0xfd469501, 22)
    NNL_MD5_STEP(NNL_MD5_F, a, b, c, d, NNL_MD5_SET(8), 0x698098d8, 7)
    NNL_MD5_STEP(NNL_MD5_F, d, a, b, c, NNL_MD5_SET(9), 0x8b44f7af, 12)
    NNL_MD5_STEP(NNL_MD5_F, c, d, a, b, NNL_MD5_SET(10), 0xffff5bb1, 17)
    NNL_MD5_STEP(NNL_MD5_F, b, c, d, a, NNL_MD5_SET(11), 0x895cd7be, 22)
    NNL_MD5_STEP(NNL_MD5_F, a, b, c, d, NNL_MD5_SET(12), 0x6b901122, 7)
    NNL_MD5_STEP(NNL_MD5_F, d, a, b, c, NNL_MD5_SET(13), 0xfd987193, 12)
    NNL_MD5_STEP(NNL_MD5_F, c, d, a, b, NNL_MD5_SET(14), 0xa679438e, 17)
    NNL_MD5_STEP(NNL_MD5_F, b, c, d, a, NNL_MD5_SET(15), 0x49b40821, 22)

    /* Round 2 */
    NNL_MD5_STEP(NNL_MD5_G, a, b, c, d, NNL_MD5_GET(1), 0xf61e2562, 5)
    NNL_MD5_STEP(NNL_MD5_G, d, a, b, c, NNL_MD5_GET(6), 0xc040b340, 9)
    NNL_MD5_STEP(NNL_MD5_G, c, d, a, b, NNL_MD5_GET(11), 0x265e5a51, 14)
    NNL_MD5_STEP(NNL_MD5_G, b, c, d, a, NNL_MD5_GET(0), 0xe9b6c7aa, 20)
    NNL_MD5_STEP(NNL_MD5_G, a, b, c, d, NNL_MD5_GET(5), 0xd62f105d, 5)
    NNL_MD5_STEP(NNL_MD5_G, d, a, b, c, NNL_MD5_GET(10), 0x02441453, 9)
    NNL_MD5_STEP(NNL_MD5_G, c, d, a, b, NNL_MD5_GET(15), 0xd8a1e681, 14)
    NNL_MD5_STEP(NNL_MD5_G, b, c, d, a, NNL_MD5_GET(4), 0xe7d3fbc8, 20)
    NNL_MD5_STEP(NNL_MD5_G, a, b, c, d, NNL_MD5_GET(9), 0x21e1cde6, 5)
    NNL_MD5_STEP(NNL_MD5_G, d, a, b, c, NNL_MD5_GET(14), 0xc33707d6, 9)
    NNL_MD5_STEP(NNL_MD5_G, c, d, a, b, NNL_MD5_GET(3), 0xf4d50d87, 14)
    NNL_MD5_STEP(NNL_MD5_G, b, c, d, a, NNL_MD5_GET(8), 0x455a14ed, 20)
    NNL_MD5_STEP(NNL_MD5_G, a, b, c, d, NNL_MD5_GET(13), 0xa9e3e905, 5)
    NNL_MD5_STEP(NNL_MD5_G, d, a, b, c, NNL_MD5_GET(2), 0xfcefa3f8, 9)
    NNL_MD5_STEP(NNL_MD5_G, c, d, a, b, NNL_MD5_GET(7), 0x676f02d9, 14)
    NNL_MD5_STEP(NNL_MD5_G, b, c, d, a, NNL_MD5_GET(12), 0x8d2a4c8a, 20)

    /* Round 3 */
    NNL_MD5_STEP(NNL_MD5_H, a, b, c, d, NNL_MD5_GET(5), 0xfffa3942, 4)
    NNL_MD5_STEP(NNL_MD5_H, d, a, b, c, NNL_MD5_GET(8), 0x8771f681, 11)
    NNL_MD5_STEP(NNL_MD5_H, c, d, a, b, NNL_MD5_GET(11), 0x6d9d6122, 16)
    NNL_MD5_STEP(NNL_MD5_H, b, c, d, a, NNL_MD5_GET(14), 0xfde5380c, 23)
    NNL_MD5_STEP(NNL_MD5_H, a, b, c, d, NNL_MD5_GET(1), 0xa4beea44, 4)
    NNL_MD5_STEP(NNL_MD5_H, d, a, b, c, NNL_MD5_GET(4), 0x4bdecfa9, 11)
    NNL_MD5_STEP(NNL_MD5_H, c, d, a, b, NNL_MD5_GET(7), 0xf6bb4b60, 16)
    NNL_MD5_STEP(NNL_MD5_H, b, c, d, a, NNL_MD5_GET(10), 0xbebfbc70, 23)
    NNL_MD5_STEP(NNL_MD5_H, a, b, c, d, NNL_MD5_GET(13), 0x289b7ec6, 4)
    NNL_MD5_STEP(NNL_MD5_H, d, a, b, c, NNL_MD5_GET(0), 0xeaa127fa, 11)
    NNL_MD5_STEP(NNL_MD5_H, c, d, a, b, NNL_MD5_GET(3), 0xd4ef3085, 16)
    NNL_MD5_STEP(NNL_MD5_H, b, c, d, a, NNL_MD5_GET(6), 0x04881d05, 23)
    NNL_MD5_STEP(NNL_MD5_H, a, b, c, d, NNL_MD5_GET(9), 0xd9d4d039, 4)
    NNL_MD5_STEP(NNL_MD5_H, d, a, b, c, NNL_MD5_GET(12), 0xe6db99e5, 11)
    NNL_MD5_STEP(NNL_MD5_H, c, d, a, b, NNL_MD5_GET(15), 0x1fa27cf8, 16)
    NNL_MD5_STEP(NNL_MD5_H, b, c, d, a, NNL_MD5_GET(2), 0xc4ac5665, 23)

    /* Round 4 */
    NNL_MD5_STEP(NNL_MD5_I, a, b, c, d, NNL_MD5_GET(0), 0xf4292244, 6)
    NNL_MD5_STEP(NNL_MD5_I, d, a, b, c, NNL_MD5_GET(7), 0x432aff97, 10)
    NNL_MD5_STEP(NNL_MD5_I, c, d, a, b, NNL_MD5_GET(14), 0xab9423a7, 15)
    NNL_MD5_STEP(NNL_MD5_I, b, c, d, a, NNL_MD5_GET(5), 0xfc93a039, 21)
    NNL_MD5_STEP(NNL_MD5_I, a, b, c, d, NNL_MD5_GET(12), 0x655b59c3, 6)
    NNL_MD5_STEP(NNL_MD5_I, d, a, b, c, NNL_MD5_GET(3), 0x8f0ccc92, 10)
    NNL_MD5_STEP(NNL_MD5_I, c, d, a, b, NNL_MD5_GET(10), 0xffeff47d, 15)
    NNL_MD5_STEP(NNL_MD5_I, b, c, d, a, NNL_MD5_GET(1), 0x85845dd1, 21)
    NNL_MD5_STEP(NNL_MD5_I, a, b, c, d, NNL_MD5_GET(8), 0x6fa87e4f, 6)
    NNL_MD5_STEP(NNL_MD5_I, d, a, b, c, NNL_MD5_GET(15), 0xfe2ce6e0, 10)
    NNL_MD5_STEP(NNL_MD5_I, c, d, a, b, NNL_MD5_GET(6), 0xa3014314, 15)
    NNL_MD5_STEP(NNL_MD5_I, b, c, d, a, NNL_MD5_GET(13), 0x4e0811a1, 21)
    NNL_MD5_STEP(NNL_MD5_I, a, b, c, d, NNL_MD5_GET(4), 0xf7537e82, 6)
    NNL_MD5_STEP(NNL_MD5_I, d, a, b, c, NNL_MD5_GET(11), 0xbd3af235, 10)
    NNL_MD5_STEP(NNL_MD5_I, c, d, a, b, NNL_MD5_GET(2), 0x2ad7d2bb, 15)
    NNL_MD5_STEP(NNL_MD5_I, b, c, d, a, NNL_MD5_GET(9), 0xeb86d391, 21)

    a += saved_a;
    b += saved_b;
    c += saved_c;
    d += saved_d;

    ptr += 64;
  } while (size -= 64);

  this->a_ = a;
  this->b_ = b;
  this->c_ = c;
  this->d_ = d;

  return ptr;
}

void MD5Context::Update(BufferView data_buf) noexcept {
  std::size_t size = data_buf.size();

  const unsigned char* data = data_buf.data();

  std::size_t saved_lo = this->lo_;

  if ((this->lo_ = (saved_lo + size) & 0x1fffffff) < saved_lo) this->hi_++;

  this->hi_ += size >> 29;

  std::size_t used = saved_lo & 0x3f;

  if (used) {
    std::size_t free = 64 - used;

    if (size < free) {
      std::memcpy(&this->buffer_[used], data, size);
      return;
    }

    std::memcpy(&this->buffer_[used], data, free);
    data = data + free;
    size -= free;
    Body(this->buffer_, 64);
  }

  if (size >= 64) {
    data = Body(data, size & ~(std::size_t)0x3f);
    size &= 0x3f;
  }

  std::memcpy(this->buffer_, data, size);
}

std::array<u8, 16> MD5Context::Final() noexcept {
  std::array<u8, 16> result;

  unsigned char* digest = result.data();

  std::size_t used = this->lo_ & 0x3f;

  this->buffer_[used++] = 0x80;

  std::size_t free = 64 - used;

  if (free < 8) {
    std::memset(&this->buffer_[used], 0, free);
    Body(this->buffer_, 64);
    used = 0;
    free = 64;
  }

  std::memset(&this->buffer_[used], 0, free - 8);

  this->lo_ <<= 3;
  this->buffer_[56] = this->lo_;
  this->buffer_[57] = this->lo_ >> 8;
  this->buffer_[58] = this->lo_ >> 16;
  this->buffer_[59] = this->lo_ >> 24;
  this->buffer_[60] = this->hi_;
  this->buffer_[61] = this->hi_ >> 8;
  this->buffer_[62] = this->hi_ >> 16;
  this->buffer_[63] = this->hi_ >> 24;

  Body(this->buffer_, 64);

  digest[0] = this->a_;
  digest[1] = this->a_ >> 8;
  digest[2] = this->a_ >> 16;
  digest[3] = this->a_ >> 24;
  digest[4] = this->b_;
  digest[5] = this->b_ >> 8;
  digest[6] = this->b_ >> 16;
  digest[7] = this->b_ >> 24;
  digest[8] = this->c_;
  digest[9] = this->c_ >> 8;
  digest[10] = this->c_ >> 16;
  digest[11] = this->c_ >> 24;
  digest[12] = this->d_;
  digest[13] = this->d_ >> 8;
  digest[14] = this->d_ >> 16;
  digest[15] = this->d_ >> 24;

  *this = {};

  return result;
}

std::array<u8, 16> MD5(BufferView data) noexcept {
  MD5Context ctx;

  ctx.Update(data);
  return ctx.Final();
}

/*
 * xxHash - Extremely Fast Hash algorithm
 * Header File
 * Copyright (C) 2012-2021 Yann Collet
 *
 * BSD 2-Clause License (https://www.opensource.org/licenses/bsd-license.php)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following disclaimer
 *      in the documentation and/or other materials provided with the
 *      distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * You can contact the author at:
 *   - xxHash homepage: https://www.xxhash.com
 *   - xxHash source repository: https://github.com/Cyan4973/xxHash
 */

struct XXH32_state_t {
  u32 total_len_32;
  u32 large_len;
  u32 v[4];
  u32 mem32[4];
  u32 memsize;
  u32 reserved;
}; /* typedef'd to XXH32_state_t */

#define XXH_PRIME32_1 0x9E3779B1U /*!< 0b10011110001101110111100110110001 */
#define XXH_PRIME32_2 0x85EBCA77U /*!< 0b10000101111010111100101001110111 */
#define XXH_PRIME32_3 0xC2B2AE3DU /*!< 0b11000010101100101010111000111101 */
#define XXH_PRIME32_4 0x27D4EB2FU /*!< 0b00100111110101001110101100101111 */
#define XXH_PRIME32_5 0x165667B1U /*!< 0b00010110010101100110011110110001 */
#define XXH_rotl32(x, r) (((x) << (r)) | ((x) >> (32 - (r))))

void XXH32_reset(XXH32_state_t* statePtr, u32 seed) {
  assert(statePtr != NULL);
  memset(statePtr, 0, sizeof(*statePtr));
  statePtr->v[0] = seed + XXH_PRIME32_1 + XXH_PRIME32_2;
  statePtr->v[1] = seed + XXH_PRIME32_2;
  statePtr->v[2] = seed + 0;
  statePtr->v[3] = seed - XXH_PRIME32_1;
}

inline u32 XXH_readLE32(const void* memPtr) {
  u32 val;
  std::memcpy(&val, memPtr, sizeof(val));
  return val;
}

inline u32 XXH32_round(u32 acc, u32 input) {
  acc += input * XXH_PRIME32_2;
  acc = XXH_rotl32(acc, 13);
  acc *= XXH_PRIME32_1;
  return acc;
}

int XXH32_update(XXH32_state_t* state, const void* input, size_t len) {
  if (input == NULL) {
    assert(len == 0);
    return 0;
  }

  {
    const u8* p = (const u8*)input;
    const u8* const bEnd = p + len;

    state->total_len_32 += (u32)len;
    state->large_len |= (u32)((len >= 16) | (state->total_len_32 >= 16));

    if (state->memsize + len < 16) { /* fill in tmp buffer */
      std::memcpy((u8*)(state->mem32) + state->memsize, input, len);
      state->memsize += (u32)len;
      return 0;
    }

    if (state->memsize) { /* some data left from previous update */
      std::memcpy((u8*)(state->mem32) + state->memsize, input, 16 - state->memsize);
      {
        const u32* p32 = state->mem32;
        state->v[0] = XXH32_round(state->v[0], XXH_readLE32(p32));
        p32++;
        state->v[1] = XXH32_round(state->v[1], XXH_readLE32(p32));
        p32++;
        state->v[2] = XXH32_round(state->v[2], XXH_readLE32(p32));
        p32++;
        state->v[3] = XXH32_round(state->v[3], XXH_readLE32(p32));
      }
      p += 16 - state->memsize;
      state->memsize = 0;
    }

    if (p <= bEnd - 16) {
      const u8* const limit = bEnd - 16;

      do {
        state->v[0] = XXH32_round(state->v[0], XXH_readLE32(p));
        p += 4;
        state->v[1] = XXH32_round(state->v[1], XXH_readLE32(p));
        p += 4;
        state->v[2] = XXH32_round(state->v[2], XXH_readLE32(p));
        p += 4;
        state->v[3] = XXH32_round(state->v[3], XXH_readLE32(p));
        p += 4;
      } while (p <= limit);
    }

    if (p < bEnd) {
      std::memcpy(state->mem32, p, (size_t)(bEnd - p));
      state->memsize = (unsigned)(bEnd - p);
    }
  }

  return 0;
}

inline u32 XXH32_avalanche(u32 hash) {
  hash ^= hash >> 15;
  hash *= XXH_PRIME32_2;
  hash ^= hash >> 13;
  hash *= XXH_PRIME32_3;
  hash ^= hash >> 16;
  return hash;
}

u32 XXH32_finalize(u32 hash, const u8* ptr, size_t len) {
#define XXH_PROCESS1                             \
  do {                                           \
    hash += (*ptr++) * XXH_PRIME32_5;            \
    hash = XXH_rotl32(hash, 11) * XXH_PRIME32_1; \
  } while (0)

#define XXH_PROCESS4                             \
  do {                                           \
    hash += XXH_readLE32(ptr) * XXH_PRIME32_3;   \
    ptr += 4;                                    \
    hash = XXH_rotl32(hash, 17) * XXH_PRIME32_4; \
  } while (0)

  if (ptr == NULL) assert(len == 0);

  /* Compact rerolled version; generally faster */

  len &= 15;
  while (len >= 4) {
    XXH_PROCESS4;
    len -= 4;
  }
  while (len > 0) {
    XXH_PROCESS1;
    --len;
  }
  return XXH32_avalanche(hash);
}

u32 XXH32_digest(const XXH32_state_t* state) {
  u32 h32;

  if (state->large_len) {
    h32 = XXH_rotl32(state->v[0], 1) + XXH_rotl32(state->v[1], 7) + XXH_rotl32(state->v[2], 12) +
          XXH_rotl32(state->v[3], 18);
  } else {
    h32 = state->v[2] /* == seed */ + XXH_PRIME32_5;
  }

  h32 += state->total_len_32;

  return XXH32_finalize(h32, (const u8*)state->mem32, state->memsize);
}

u32 XXH32(BufferView data, u32 seed) {
  XXH32_state_t state;
  XXH32_reset(&state, seed);
  XXH32_update(&state, data.data(), data.size());
  return XXH32_digest(&state);
}

}  // namespace utl::data

}  // namespace nnl
