#include "NNL/utility/string.hpp"

#include "NNL/common/contract.hpp"
#include "NNL/utility/utf8.hpp"
namespace nnl {
namespace utl::string {

bool EndsWith(std::string_view str, std::string_view suffix) {
  return str.size() >= suffix.size() && 0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix);
}

bool StartsWith(std::string_view str, std::string_view prefix) {
  return str.size() >= prefix.size() && 0 == str.compare(0, prefix.size(), prefix);
}

std::string Join(const std::vector<std::string>& strings, std::string delim) {
  std::string accum;

  if (strings.empty()) return accum;

  accum.reserve(16 * strings.size());

  for (std::size_t i = 0; i < strings.size() - 1; i++) {
    accum += (strings[i] + delim);
  }

  accum += strings.back();

  return accum;
}

std::vector<std::string> Split(std::string_view str, std::string delimiter) {
  size_t pos_start = 0, pos_end, delim_len = delimiter.length();
  std::string token;
  std::vector<std::string> res;

  while ((pos_end = str.find(delimiter, pos_start)) != std::string::npos) {
    token = str.substr(pos_start, pos_end - pos_start);
    pos_start = pos_end + delim_len;
    res.push_back(token);
  }

  res.push_back(std::string(str.substr(pos_start)));
  return res;
}

std::string Truncate(std::string_view str, std::size_t n) { return std::string(str.substr(0, n)); }

std::string FloatToString(float value, int n) {
  std::ostringstream out;
  out.precision(n);
  out << std::fixed << value;
  return out.str();
}

std::string ToLower(std::string_view str) {
  std::string str_lower;

  str_lower.resize(str.size());

  std::transform(str.begin(), str.end(), str_lower.begin(),
                 [](unsigned char c) { return c <= 127 ? std::tolower(c) : c; });
  return str_lower;
}

bool CompareNat(std::string_view a, std::string_view b) {
  NNL_EXPECTS_DBG(utl::utf8::IsValid(a));
  NNL_EXPECTS_DBG(utl::utf8::IsValid(b));

  auto is_digit = [](std::size_t c) -> bool {
    if (c >= '0' && c <= '9') return true;
    return false;
  };

  if (a.empty() && !b.empty()) return true;
  if (b.empty() && !a.empty()) return false;
  if (a.empty() && b.empty()) return false;

  std::size_t code_0_size = utl::utf8::GetSize(a);
  std::size_t code_1_size = utl::utf8::GetSize(b);

  char32_t code_0 = utl::utf8::Decode(a);

  char32_t code_1 = utl::utf8::Decode(b);

  if (is_digit(code_0) && !is_digit(code_1)) return true;

  if (!is_digit(code_0) && is_digit(code_1)) return false;

  if (!is_digit(code_0) && !is_digit(code_1)) {
    if (code_0 == code_1) return CompareNat(a.substr(code_0_size), b.substr(code_1_size));
    return (code_0 < code_1);
  }

  std::size_t ia, ib, posa, posb;

  char* posa_p = nullptr;
  char* posb_p = nullptr;

  ia = std::strtoul(a.data(), &posa_p, 10);
  ib = std::strtoul(b.data(), &posb_p, 10);

  assert(posa_p != nullptr);
  assert(posb_p != nullptr);

  posa = posa_p - a.data();
  posb = posb_p - b.data();

  if (ia != ib) return ia < ib;

  return (CompareNat(a.substr(posa), b.substr(posb)));
}

}  // namespace utl::string
}  // namespace nnl
