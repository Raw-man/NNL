#include "NNL/utility/utf8.hpp"

#include "NNL/common/contract.hpp"
namespace nnl {
namespace utl::utf8 {

bool IsValid(std::string_view str) {
  for (std::size_t i = 0, ix = str.length(); i < ix; i++) {
    unsigned char c = str[i];
    std::size_t n = 0;

    if (c == 0) return false;

    if (c <= 0x7f)
      n = 0;  // 0bbbbbbb
    else if ((c & 0xE0) == 0xC0)
      n = 1;  // 110bbbbb
    else if (c == 0xed && i < (ix - 1) && ((unsigned char)str[i + 1] & 0xa0) == 0xa0)
      return false;  // U+d800 to U+dfff
    else if ((c & 0xF0) == 0xE0)
      n = 2;  // 1110bbbb
    else if ((c & 0xF8) == 0xF0)
      n = 3;  // 11110bbb
    else
      return false;

    for (std::size_t j = 0; j < n && i < ix; j++)  // n bytes matching 10bbbbbb follow ?
    {
      if ((++i == ix) || (((unsigned char)str[i] & 0xC0) != 0x80)) return false;
    }
  }
  return true;
}

std::size_t GetSize(std::string_view str, std::size_t pos) {
  if (str.empty() || pos >= str.size()) return 0;

  if (0xf0 == (0xf8 & str[pos])) {
    /* 4 byte utf8 codepoint */
    return 4;
  } else if (0xe0 == (0xf0 & str[pos])) {
    /* 3 byte utf8 codepoint */
    return 3;
  } else if (0xc0 == (0xe0 & str[pos])) {
    /* 2 byte utf8 codepoint */
    return 2;
  }

  /* 1 byte utf8 codepoint otherwise */
  return 1;
}

char32_t Decode(std::string_view str, std::size_t pos) {
  NNL_EXPECTS_DBG(utl::utf8::IsValid(str.substr(pos, utl::utf8::GetSize(str, pos))));
  if (0xf0 == (0xf8 & str[pos + 0])) {
    /* 4 byte utf8 codepoint */
    return str.size() < 4 ? 0xFFFD
                          : ((0x07 & str[pos + 0]) << 18) | ((0x3f & str[pos + 1]) << 12) |
                                ((0x3f & str[pos + 2]) << 6) | (0x3f & str[pos + 3]);

  } else if (0xe0 == (0xf0 & str[pos + 0])) {
    /* 3 byte utf8 codepoint */
    return str.size() < 3 ? 0xFFFD
                          : ((0x0f & str[pos + 0]) << 12) | ((0x3f & str[pos + 1]) << 6) | (0x3f & str[pos + 2]);

  } else if (0xc0 == (0xe0 & str[pos + 0])) {
    /* 2 byte utf8 codepoint */
    return str.size() < 2 ? 0xFFFD : ((0x1f & str[pos + 0]) << 6) | (0x3f & str[pos + 1]);
    // str += 2;
  } else {
    /* 1 byte utf8 codepoint otherwise */
    return str.empty() ? 0xFFFD : str[pos + 0];
  }
}

std::string Encode(char32_t codepoint) {
  NNL_EXPECTS_DBG(codepoint <= 0x10FFFD);
  std::string str;

  if (0 == ((std::size_t)0xffffff80 & codepoint)) {
    /* 1-byte/7-bit ascii
     * (0b0xxxxxxx) */

    str += (char)codepoint;

  } else if (0 == ((std::size_t)0xfffff800 & codepoint)) {
    /* 2-byte/11-bit utf8 code point
     * (0b110xxxxx 0b10xxxxxx) */
    str += (char)(0xc0 | (char)((codepoint >> 6) & 0x1f));
    str += (char)(0x80 | (char)(codepoint & 0x3f));

  } else if (0 == ((std::size_t)0xffff0000 & codepoint)) {
    /* 3-byte/16-bit utf8 code point
     * (0b1110xxxx 0b10xxxxxx 0b10xxxxxx) */

    str += (char)(0xe0 | (char)((codepoint >> 12) & 0x0f));
    str += (char)(0x80 | (char)((codepoint >> 6) & 0x3f));
    str += (char)(0x80 | (char)(codepoint & 0x3f));

  } else { /* if (0 == ((int)0xffe00000 & chr)) { */
    /* 4-byte/21-bit utf8 code point
     * (0b11110xxx 0b10xxxxxx 0b10xxxxxx 0b10xxxxxx) */

    str += (char)(0xf0 | (char)((codepoint >> 18) & 0x07));
    str += (char)(0x80 | (char)((codepoint >> 12) & 0x3f));
    str += (char)(0x80 | (char)((codepoint >> 6) & 0x3f));
    str += (char)(0x80 | (char)(codepoint & 0x3f));
  }

  return str;
}

bool IsRightToLeft(char32_t codepoint) {
  NNL_EXPECTS_DBG(codepoint <= 0x10FFFD);
  if ((codepoint == 0x05BE) || (codepoint == 0x05C0) || (codepoint == 0x05C3) || (codepoint == 0x05C6) ||
      ((codepoint >= 0x05D0) && (codepoint <= 0x05F4)) || (codepoint == 0x0608) || (codepoint == 0x060B) ||
      (codepoint == 0x060D) || ((codepoint >= 0x061B) && (codepoint <= 0x064A)) ||
      ((codepoint >= 0x066D) && (codepoint <= 0x066F)) || ((codepoint >= 0x0671) && (codepoint <= 0x06D5)) ||
      ((codepoint >= 0x06E5) && (codepoint <= 0x06E6)) || ((codepoint >= 0x06EE) && (codepoint <= 0x06EF)) ||
      ((codepoint >= 0x06FA) && (codepoint <= 0x0710)) || ((codepoint >= 0x0712) && (codepoint <= 0x072F)) ||
      ((codepoint >= 0x074D) && (codepoint <= 0x07A5)) || ((codepoint >= 0x07B1) && (codepoint <= 0x07EA)) ||
      ((codepoint >= 0x07F4) && (codepoint <= 0x07F5)) || ((codepoint >= 0x07FA) && (codepoint <= 0x0815)) ||
      (codepoint == 0x081A) || (codepoint == 0x0824) || (codepoint == 0x0828) ||
      ((codepoint >= 0x0830) && (codepoint <= 0x0858)) || ((codepoint >= 0x085E) && (codepoint <= 0x08AC)) ||
      (codepoint == 0x200F) || (codepoint == 0x202E) || (codepoint == 0xFB1D) ||
      ((codepoint >= 0xFB1F) && (codepoint <= 0xFB28)) || ((codepoint >= 0xFB2A) && (codepoint <= 0xFD3D)) ||
      ((codepoint >= 0xFD50) && (codepoint <= 0xFDFC)) || ((codepoint >= 0xFE70) && (codepoint <= 0xFEFC)) ||
      ((codepoint >= 0x10800) && (codepoint <= 0x1091B)) || ((codepoint >= 0x10920) && (codepoint <= 0x10A00)) ||
      ((codepoint >= 0x10A10) && (codepoint <= 0x10A33)) || ((codepoint >= 0x10A40) && (codepoint <= 0x10B35)) ||
      ((codepoint >= 0x10B40) && (codepoint <= 0x10C48)) || ((codepoint >= 0x1EE00) && (codepoint <= 0x1EEBB)))
    return true;
  return false;
}

bool IsASCII(std::string_view str) {
  for (unsigned char c : str) {
    if (c > 127) return false;
  }
  return true;
}

}  // namespace utl::utf8
}  // namespace nnl
