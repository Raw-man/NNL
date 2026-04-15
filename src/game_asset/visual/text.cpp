#include "NNL/game_asset/visual/text.hpp"

#include "NNL/common/logger.hpp"
#include "NNL/utility/math.hpp"
#include "NNL/utility/string.hpp"
#include "NNL/utility/utf8.hpp"
#include "stb_truetype.h"
namespace nnl {

namespace text {

std::vector<std::string> Convert(const Text& text, const std::unordered_map<u16, std::string_view>& replacements) {
  std::vector<std::string> stext;

  stext.reserve(text.strings.size());

  for (auto& paragraph : text.strings) {
    auto& str = stext.emplace_back();

    str.reserve(paragraph.size() * 4);

    for (auto character_index : paragraph) {
      if ((character_index & kSpecialCodeMask) == 0) {
        str += utl::utf8::Encode(text.characters.at(character_index));
      } else if (replacements.find(character_index) != replacements.end()) {
        assert(utl::utf8::IsValid(replacements.at(character_index)));
        str += replacements.at(character_index);

      } else {
        NNL_LOG_WARN("character ignored: " + utl::string::IntToHex(character_index, true));
      }
    }
  }

  return stext;
}

Text Convert(const std::vector<std::string>& stext, const std::unordered_map<u16, std::string_view>& replacements,
             const std::vector<u16>& characters) {
  Text text;

  std::map<u16, u16> character_map;  // character / index

  if (!characters.empty()) text.characters = characters;

  for (std::size_t i = 0; i < characters.size(); i++) character_map[characters[i]] = i;

  for (const std::string_view str : stext) {
    NNL_EXPECTS_DBG(utl::utf8::IsValid(str));
    bool right_to_left = false;

    auto& paragraph = text.strings.emplace_back();

    paragraph.reserve(str.size() * sizeof(u16));

    for (std::size_t i = 0; i < str.size();) {
      assert(i < str.size());
      // See if a substring should be replaced
      bool replaced = false;

      for (auto& [key, value] : replacements) {
        if (!value.empty() && i + value.size() <= str.size() && value == str.substr(i, value.size())) {
          paragraph.push_back(key);
          i += value.size();
          replaced = true;
          break;
        }
      }

      if (replaced) continue;

      u32 utf8_size = utl::utf8::GetSize(str, i);

      std::string_view utf8code = str.substr(i, utf8_size);

      i += utf8_size;

      char32_t codepoint = utl::utf8::Decode(utf8code);

      if (utf8_size > 3) {
        NNL_LOG_WARN("codepoint is outside of the basic multilingual plane: U+" +
                     utl::string::IntToHex(codepoint).substr(2));
        continue;
      }

      u16 ucs2code = (u16)codepoint;

      if ((ucs2code == 0xFEFF) || (ucs2code >= 0xFE00 && ucs2code <= 0xFE0F)  // variation selector
          || (ucs2code <= 8)                                                  // control characters
          || (ucs2code >= 12 && ucs2code <= 31) || (ucs2code == 0x200E) || (ucs2code == 0x200F))
        continue;  // rtl, ltr marks

      if (utl::utf8::IsRightToLeft(ucs2code)) right_to_left = true;

      if (character_map.find(ucs2code) == character_map.end()) {
        if (!characters.empty()) {
          NNL_LOG_WARN("codepoint was not found in the provided character set: U+" + utl::string::IntToHex(ucs2code));
          continue;
        }

        if (text.characters.size() >= 0xFFF) NNL_THROW(RuntimeError(NNL_SRCTAG("the limit of characters exceeded")));

        character_map[ucs2code] = text.characters.size();
        paragraph.push_back(text.characters.size());
        text.characters.push_back(ucs2code);

      } else {
        paragraph.push_back(character_map[ucs2code]);
      }
    }

    // Reverse order of bytes for right_to_left characters
    if (right_to_left && paragraph.size() > 0) {
      auto begin = paragraph.begin();
      auto end = begin;

      while (end != paragraph.end()) {
        begin = std::find_if_not(end, paragraph.end(), [](auto ch) { return ch == 0x80'01; });

        end = std::find_if(begin, paragraph.end(), [](auto ch) { return ch == 0x80'01; });

        if (begin != paragraph.end() && end <= paragraph.end()) {
          std::reverse(begin, end);
        }
      }
    }
    // Some symbol codes are made of 2 parts: insert it if missing
    for (std::size_t i = 0; i < paragraph.size(); i++) {
      u16 special_character = paragraph.at(i);
      if (special_character > 0x1007 && special_character < 0x1010 && special_character % 2 == 0) {
        if (i + 1 == paragraph.size() || (i + 1 < paragraph.size() && paragraph.at(i + 1) != special_character + 1)) {
          paragraph.insert(paragraph.begin() + i + 1, special_character + 1);
          i++;
        }
      }
    }
    // Insert string terminator
    if (paragraph.size() == 0 || paragraph.back() != 0x8000) {
      paragraph.push_back(0x8000);
    }
  }

  return text;
}

std::string GenerateFNT(const Text& text, const std::vector<u8>& advance_width, const std::vector<STexture>& bitmaps,
                        int columns) {
  NNL_EXPECTS(!bitmaps.empty());
  NNL_EXPECTS(columns == -1 || utl::math::IsPow2(columns));
  if (columns == -1) {
    std::size_t max_width = 16;

    for (auto advance : advance_width) max_width = std::max<std::size_t>(max_width, advance);

    max_width = utl::math::RoundUpPow2(max_width);

    columns = bitmaps.at(0).width / max_width;
  }

  std::ostringstream out;
  std::size_t bitmap_width = bitmaps.at(0).width;
  std::size_t character_width = bitmaps.at(0).width / (std::size_t)columns;

  out << "info face=\"\" size=" << character_width
      << " bold=0 italic=0 charset=\"\" unicode=1 stretchH=100 smooth=1 aa=1 "
         "padding=0,0,0,0 spacing=0,0 outline=0\n";
  out << "common lineHeight=0 base=" << character_width << " scaleW=" << bitmap_width << " scaleH=" << bitmap_width
      << " pages=" << bitmaps.size() << " packed=0 alphaChnl=0 redChnl=0 greenChnl=0 blueChnl=0\n";

  for (std::size_t i = 0; i < bitmaps.size(); i++) {
    out << "page id=" << i << " file=\"" << bitmaps.at(i).name << ".png\"\n";
  }

  // fake space
  out << "chars count=" << text.characters.size() + 1 << "\n";

  out << "char id=32   x=0     y=0     width=0    height=0    xoffset=0   "
         "yoffset=0     xadvance="
      << character_width / 2 << "    page=0  chnl=15\n";

  for (std::size_t i = 0; i < text.characters.size(); i++) {
    out << "char id=" << text.characters.at(i) << " x=" << character_width * (i % columns)
        << "     y=" << character_width * ((i / columns) % columns) << "     width=" << character_width
        << "    height=" << character_width
        << "    xoffset=0    yoffset=0     xadvance=" << (i16)advance_width.at(i) + 2
        << "     page=" << i / (columns * columns) << "  chnl=15\n";
  }

  return out.str();
}

BitmapFont _GenerateBitmapFont(Text& text, BufferView ttf_font_file, const BitmapFontParams& params) {
  stbtt_fontinfo info;

  NNL_EXPECTS(!text.characters.empty());

  NNL_EXPECTS(utl::math::IsPow2(params.columns));

  NNL_EXPECTS(utl::math::IsPow2(params.size));

  NNL_EXPECTS(params.size >= 128 && params.size <= 1024);

  NNL_EXPECTS(params.scale_factor > 0.0f);

  NNL_EXPECTS(params.opacity_factor >= 0.0f);

  NNL_EXPECTS(params.spacing_offset > -128 && params.spacing_offset <= 127);

  if (!stbtt_InitFont(&info, ttf_font_file.data(), 0)) {
    NNL_THROW(RuntimeError(NNL_SRCTAG("failed to initialize ttf font")));
  }

  auto alpha_levels = std::clamp(params.alpha_levels, 2u, 256u) - 1;

  for (std::size_t i = 0; i < text.characters.size(); ++i) {
    if (stbtt_FindGlyphIndex(&info, text.characters.at(i)) == 0) {
      NNL_LOG_WARN("the glyph for a codepoint was not found in the font: U+" +
                   utl::string::IntToHex(text.characters.at(i)));
    }
  }

  std::vector<u8> spacing;

  spacing.reserve(text.characters.size());

  std::vector<STexture> stextures;

  const int b_w = params.size;                                              /* bitmap width */
  const int b_h = params.size;                                              /* bitmap height */
  const int l_h = std::round((b_h / params.columns) * params.scale_factor); /* line height */

  /* create a bitmap for the phrase */
  std::vector<unsigned char*> bitmaps;

  unsigned char* bitmap = nullptr;

  /* calculate font scaling */

  int x = 0;
  int y = 0;
  int ascent, descent, lineGap;
  stbtt_GetFontVMetrics(&info, &ascent, &descent, &lineGap);

  float scale = stbtt_ScaleForPixelHeight(&info, l_h);

  std::vector<int> character_pair;

  if (params.simulate_kerning) {
    std::map<u64, u16> characters;  // character pair / index

    Text new_text;

    for (auto& paragraph : text.strings) {
      auto& new_paragraph = new_text.strings.emplace_back();

      for (std::size_t i = 0; i < paragraph.size(); i++) {
        auto character_index_1 = paragraph.at(i);

        // Copy a special code as it is
        if ((character_index_1 & kSpecialCodeMask) != 0) {
          new_paragraph.push_back(character_index_1);
          continue;
        };

        auto character_index_2 = paragraph.at(i + 1);

        auto ucs2code_1 = text.characters.at(character_index_1);

        int ax;
        int lsb;

        int kern = 0;

        stbtt_GetCodepointHMetrics(&info, ucs2code_1, &ax, &lsb);

        if ((character_index_2 & kSpecialCodeMask) == 0) {
          auto ucs2code_2 = text.characters.at(character_index_2);

          kern = stbtt_GetCodepointKernAdvance(&info, ucs2code_1, ucs2code_2);
        }

        kern = static_cast<i32>(std::floor(static_cast<float>(kern) * scale + static_cast<float>(ax) * scale)) +
               params.spacing_offset;

        u64 ucs2code_pair = (u64)kern << 32 | (u64)ucs2code_1;

        if (characters.find(ucs2code_pair) == characters.end()) {
          characters[ucs2code_pair] = new_text.characters.size();
          new_paragraph.push_back(new_text.characters.size());
          new_text.characters.push_back(ucs2code_1);
          character_pair.push_back(kern);
        } else {
          new_paragraph.push_back(characters[ucs2code_pair]);
        }
      }
    }

    text = std::move(new_text);
  }

  ascent = std::round(ascent * scale);
  descent = std::round(descent * scale);

  std::size_t char_counter = -1;

  for (std::size_t i = 0; i < text.characters.size(); ++i) {
    if (char_counter >= (params.columns * params.columns)) {
      x = 0;
      y = 0;
      char_counter = 0;
      bitmap = new unsigned char[b_w * b_h]{};
      bitmaps.push_back(bitmap);
    }

    /* how wide is this character */
    int ax;  // advance x
    int lsb;

    stbtt_GetCodepointHMetrics(&info, text.characters.at(i), &ax, &lsb);
    /* (Note that each Codepoint call has an alternative Glyph version which
     * caches the work required to lookup the character word[i].) */

    /* get bounding box for character (may be offset to account for chars that
     * dip above or below the line) */
    int c_x1, c_y1, c_x2, c_y2;
    stbtt_GetCodepointBitmapBox(&info, text.characters.at(i), scale, scale, &c_x1, &c_y1, &c_x2, &c_y2);

    int shift_y = std::max(0, ascent + c_y1);
    int byteOffset = x + ((y + shift_y) * b_w);

    stbtt_MakeCodepointBitmap(&info, bitmap + byteOffset, (b_w / params.columns), (b_h / params.columns) - (shift_y),
                              b_w, scale, scale, text.characters.at(i));

    char_counter++;

    x += b_w / params.columns;

    if (x >= b_w) {
      x = 0;
      y += b_h / params.columns;
    }

    /* add kerning */

    if (params.simulate_kerning)
      spacing.push_back(character_pair.at(i));
    else
      spacing.push_back(std::floor(ax * scale) + params.spacing_offset);

    // kerning.push_back(tracking+c_x2-c_x1);
  }

  /* save out a 1 channel image */
  //
  for (auto bitmap : bitmaps) {
    STexture& stexture = stextures.emplace_back();

    stexture.width = b_w;
    stexture.height = b_h;

    if (!params.filter_nearest) {
      stexture.min_filter = STextureFilter::kLinear;
      stexture.mag_filter = STextureFilter::kLinear;
    } else {
      stexture.min_filter = STextureFilter::kNearest;
      stexture.mag_filter = STextureFilter::kNearest;
    }

    stexture.bitmap.reserve(stexture.width * stexture.height);

    float step = 255.0f / alpha_levels;

    for (std::size_t i = 0; i < (stexture.width * stexture.height); i++) {
      u8 alpha_color = (u8)std::clamp(std::round(bitmap[i] * params.opacity_factor / step) * step, 0.0f, 255.0f);

      if (alpha_color > 0)
        stexture.bitmap.push_back({0xFF, 0xFF, 0xFF, alpha_color});
      else
        stexture.bitmap.push_back({0xEF, 0xEF, 0xEF, 0});
    }

    delete[] bitmap;
  }

  if (!bitmaps.empty()) {
    auto& last_bitmap = stextures.back();

    last_bitmap.height = std::min(y + b_h / params.columns, (unsigned int)b_h);

    last_bitmap.bitmap.resize(last_bitmap.width * last_bitmap.height);
  }
  /*
   Note that this example writes each character directly into the target image
   buffer. The "right thing" to do for fonts that have overlapping characters is
   MakeCodepointBitmap to a temporary buffer and then alpha blend that onto the
   target image. See the stb_truetype.h header for more info.
  */

  return {stextures, spacing};
}

BitmapFont GenerateBitmapFont(Text& text, BufferView ttf_font_file, const BitmapFontParams& params) {
  return _GenerateBitmapFont(text, ttf_font_file, params);
}

BitmapFont GenerateBitmapFont(Text& text, const std::filesystem::path& ttf_font_path, const BitmapFontParams& params) {
  FileReader f{ttf_font_path};
  Buffer font = f.ReadArrayLE<u8>(f.Len());
  return _GenerateBitmapFont(text, font, params);
}

bool IsOfType_(Reader& f) {
  using namespace raw;

  f.Seek(0);

  const std::size_t data_size = f.Len();

  if (data_size < sizeof(u32)) return false;

  u32 num_phrases = f.ReadLE<u32>();

  if (num_phrases == 0) return false;

  f.Seek(sizeof(u32) + num_phrases * sizeof(u32));

  if (data_size < f.Tell() + sizeof(u32)) return false;

  u32 offset_cdl = f.ReadLE<u32>();

  if (data_size < offset_cdl + sizeof(u32)) return false;

  f.Seek(offset_cdl);

  u32 magic_bytes = f.ReadLE<u32>();

  if (magic_bytes != kMagicBytes) return false;

  return true;
}

bool IsOfType(Reader& f) { return IsOfType_(f); }

bool IsOfType(BufferView buffer) { return IsOfType_(buffer); }

bool IsOfType(const std::filesystem::path& path) {
  FileReader f{path};
  return IsOfType_(f);
}

namespace raw {

Text Convert(RText&& rtext) {
  Text text;
  text.characters = std::move(rtext.cdl.characters);
  text.strings.reserve(rtext.header.num_strings);
  for (auto& [offset, phrase] : rtext.header.strings) text.strings.push_back(std::move(phrase));

  rtext = {};

  return text;
}

RText Parse(Reader& f) {
  f.Seek(0);
  RText rtext;
  auto& header = rtext.header;

  NNL_TRY {
    header.num_strings = f.ReadLE<u32>();
    header.offset_strings = f.ReadArrayLE<u32>(header.num_strings);
    header.offset_cdl = f.ReadLE<u32>();

    auto& cdl = rtext.cdl;

    f.Seek(header.offset_cdl);

    cdl.magic_bytes = f.ReadLE<u32>();

    if (cdl.magic_bytes != kMagicBytes) NNL_THROW(ParseError(NNL_SRCTAG("invalid magic bytes")));

    cdl.num_characters = f.ReadLE<u32>();
    cdl.num_0 = f.ReadLE<u32>();
    cdl.num_1 = f.ReadLE<u32>();
    cdl.characters = f.ReadArrayLE<u16>(cdl.num_characters);

    for (auto& offset_phrase : header.offset_strings) {
      f.Seek(offset_phrase);

      std::vector<u16> phrase;

      do {
        phrase.push_back(f.ReadLE<u16>());
      } while (phrase.back() != 0x80'00);

      header.strings.insert({offset_phrase, std::move(phrase)});
    }
  }
  NNL_CATCH(std::exception) { NNL_THROW(ParseError{NNL_SRCTAG(e.what())}); }
  return rtext;
}

}  // namespace raw

Text Import_(Reader& f) { return raw::Convert(raw::Parse(f)); }

Text Import(Reader& f) { return Import_(f); }

Text Import(BufferView buffer) { return Import_(buffer); }

Text Import(const std::filesystem::path& path) {
  FileReader f{path};

  return Import_(f);
}

void Export_(const Text& text, Writer& f) {
  NNL_EXPECTS(!text.strings.empty());

  f.Seek(0);

  f.WriteLE<u32>(utl::data::narrow<u32>(text.strings.size()));

  auto offset_phrases_start = f.MakeOffsetLE<u32>();

  std::vector<u32> offsets(text.strings.size(), 0);

  f.WriteArrayLE(offsets);

  auto offset_cdl = f.WriteLE<u32>(0);

  for (auto& phrase : text.strings) {
    *offset_phrases_start = utl::data::narrow<u32>(f.Tell());
    f.WriteArrayLE(phrase);
    ++offset_phrases_start;
  }

  f.AlignData(0x10, 0);

  *offset_cdl = utl::data::narrow<u32>(f.Tell());

  f.WriteLE<u32>(raw::kMagicBytes);

  f.WriteLE(utl::data::narrow<u32>(text.characters.size()));

  f.AlignData(0x10, 0);

  f.WriteArrayLE(text.characters);
}

Buffer Export(const Text& text) {
  BufferRW f;
  Export_(text, f);
  return f;
}

void Export(const Text& text, Writer& f) { Export_(text, f); }

void Export(const Text& text, const std::filesystem::path& path) {
  FileRW f{path, true};
  Export_(text, f);
}

}  // namespace text

}  // namespace nnl
