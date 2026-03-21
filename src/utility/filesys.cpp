#include "NNL/utility/filesys.hpp"

#include "NNL/common/contract.hpp"
#include "NNL/utility/utf8.hpp"
namespace nnl {
namespace utl::filesys {
std::string u8string(const std::filesystem::path& path) {
#ifdef __cpp_char8_t  // c++20
  auto u8str = path.u8string();
  return std::string(u8str.begin(), u8str.end());
#else
  return path.u8string();
#endif
}

std::filesystem::path u8path(std::string_view path) {
  NNL_EXPECTS_DBG(utl::utf8::IsValid(path));
#ifdef __cpp_char8_t  // c++20
  auto ptr_begin = reinterpret_cast<const char8_t*>(path.data());
  return std::filesystem::path(ptr_begin, ptr_begin + path.size());
#else
  return std::filesystem::u8path(path);
#endif
}

std::filesystem::path ReplaceExtension(const std::filesystem::path& path, const std::filesystem::path& new_extension) {
  auto new_path = path;
  return new_path.replace_extension(new_extension);
}

}  // namespace utl::file
}  // namespace nnl
