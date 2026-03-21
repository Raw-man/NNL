#include "NNL/game_asset/audio/adpcm.hpp"

namespace nnl {
namespace adpcm {

Buffer Encode(const std::vector<i16> &pcm) {
  const static double f[5][2] = {{0.0, 0.0},
                                 {-60.0 / 64.0, 0.0},
                                 {-115.0 / 64.0, 52.0 / 64.0},
                                 {-98.0 / 64.0, 55.0 / 64.0},
                                 {-122.0 / 64.0, 60.0 / 64.0}};

  double s_1 = 0.0;
  double s_2 = 0.0;

  auto pack = [&s_1, &s_2](double *d_samples, short *four_bit, int predict_nr, int shift_factor) {
    double ds;
    int di;
    double s_0;
    int i;

    for (i = 0; i < 28; i++) {
      s_0 = d_samples[i] + s_1 * f[predict_nr][0] + s_2 * f[predict_nr][1];
      ds = s_0 * (double)(1 << shift_factor);

      di = ((int)ds + 0x800) & 0xfffff000;

      if (di > 32767) di = 32767;
      if (di < -32768) di = -32768;

      four_bit[i] = (short)di;

      di = di >> shift_factor;
      s_2 = s_1;
      s_1 = (double)di - s_0;
    }
  };

  double _s_1 = 0.0;  // s[t-1]
  double _s_2 = 0.0;  // s[t-2]

  auto find_predict = [&_s_1, &_s_2](short *samples, double *d_samples, int *predict_nr, int *shift_factor) {
    int i, j;
    double buffer[28][5];
    double min = 1e10;
    double max[5];
    double ds;
    int min2;
    int shift_mask;
    double s_0, s_1, s_2;

    for (i = 0; i < 5; i++) {
      max[i] = 0.0;
      s_1 = _s_1;
      s_2 = _s_2;
      for (j = 0; j < 28; j++) {
        s_0 = (double)samples[j];  // s[t-0]
        if (s_0 > 30719.0) s_0 = 30719.0;
        if (s_0 < -30720.0) s_0 = -30720.0;
        ds = s_0 + s_1 * f[i][0] + s_2 * f[i][1];
        buffer[j][i] = ds;
        if (fabs(ds) > max[i]) max[i] = fabs(ds);

        s_2 = s_1;  // new s[t-2]
        s_1 = s_0;  // new s[t-1]
      }

      if (max[i] < min) {
        min = max[i];
        *predict_nr = i;
      }
      if (min <= 7) {
        *predict_nr = 0;
        break;
      }
    }

    // store s[t-2] and s[t-1] in a static variable
    // these than used in the next function call

    _s_1 = s_1;
    _s_2 = s_2;

    for (i = 0; i < 28; i++) d_samples[i] = buffer[i][*predict_nr];

    //  if ( min > 32767.0 )
    //      min = 32767.0;

    min2 = (int)min;
    shift_mask = 0x4000;
    *shift_factor = 0;

    while (*shift_factor < 12) {
      if (shift_mask & (min2 + (shift_mask >> 3))) break;
      (*shift_factor)++;
      shift_mask = shift_mask >> 1;
    }
  };

  constexpr int buffer_size = 128 * 28;

  short wave[buffer_size];

  std::size_t seek = 0;

  BufferRW adpcm;
  //    if(enable_looping)
  //        flags = 6;
  //    else
  i32 flags = 0;
  int sample_len = pcm.size();
  int size = 0;

  int predict_nr = 0;
  int shift_factor = 0;
  double d_samples[28];
  short four_bit[28];

  while (sample_len > 0) {
    size = (sample_len >= buffer_size) ? buffer_size : sample_len;

    // fread( wave, sizeof( short ), size, fp );
    for (int i = 0; i < size; i++, seek++) wave[i] = pcm[seek];

    int i = size / 28;
    if (size % 28) {
      for (int j = size % 28; j < 28; j++) wave[28 * i + j] = 0;
      i++;
    }

    for (int j = 0; j < i; j++)  // pack 28 samples
    {
      short *ptr = wave + j * 28;
      find_predict(ptr, d_samples, &predict_nr, &shift_factor);
      pack(d_samples, four_bit, predict_nr, shift_factor);
      unsigned char d = (predict_nr << 4) | shift_factor;
      // fputc( d, vag );
      adpcm.WriteLE<u8>(d);
      adpcm.WriteLE<u8>(flags);
      // fputc( flags, vag );
      for (int k = 0; k < 28; k += 2) {
        d = ((four_bit[k + 1] >> 8) & 0xf0) | ((four_bit[k] >> 12) & 0xf);
        // fputc( d, vag );
        adpcm.WriteLE<u8>(d);
      }
      sample_len -= 28;
      if (sample_len < 28 /* && enable_looping == 0*/) flags = 1;

      //        if(enable_looping)
      //        flags = 2;
    }
  }

  adpcm.WriteLE<u8>((predict_nr << 4) | shift_factor);

  //    if(enable_looping)
  //        fputc(3, vag);
  //    else

  //  fputc( 7, vag );            // end flag
  adpcm.WriteLE<u8>(7);

  for (std::size_t i = 0; i < 7; i++) adpcm.WriteLE<u16>(0x77'77);
  //    for ( int i = 0; i < 14; i++ )
  //        fputc( 0, vag );

  return adpcm;
}

std::vector<i16> Decode(BufferView adpcm) {
  const double f[5][2] = {{0.0, 0.0},
                          {60.0 / 64.0, 0.0},
                          {115.0 / 64.0, -52.0 / 64.0},
                          {98.0 / 64.0, -55.0 / 64.0},
                          {122.0 / 64.0, -60.0 / 64.0}};

  double samples[28];
  std::memset(samples, 0, sizeof(samples));

  int predict_nr, shift_factor, flags;
  int i;
  int d, s;
  double s_1 = 0.0;
  double s_2 = 0.0;

  adpcm.Seek(0);

  std::vector<i16> pcm;

  pcm.reserve(adpcm.Len() * 4);

  while (adpcm.Tell() < adpcm.Len()) {
    predict_nr = adpcm.ReadLE<u8>();
    shift_factor = predict_nr & 0xf;
    predict_nr >>= 4;
    flags = adpcm.ReadLE<u8>();  // flags
    if (flags == 7) break;
    for (i = 0; i < 28; i += 2) {
      d = adpcm.ReadLE<u8>();
      s = (d & 0xf) << 12;
      if (s & 0x8000) s |= 0xffff0000;
      samples[i] = (double)(s >> shift_factor);
      s = (d & 0xf0) << 8;
      if (s & 0x8000) s |= 0xffff0000;
      samples[i + 1] = (double)(s >> shift_factor);
    }

    for (i = 0; i < 28; i++) {
      samples[i] = samples[i] + s_1 * f[predict_nr][0] + s_2 * f[predict_nr][1];
      s_2 = s_1;
      s_1 = samples[i];
      d = (int)(samples[i] + 0.5);
      pcm.push_back((u16)(d & 0xFFFF));
    }
  }

  return pcm;
}
}  // namespace adpcm
}  // namespace nnl
