/**
 * @file filesys.hpp
 * @brief Provides utility functions for filesystem operations.
 *
 */
#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace nnl {
/**
 * @copydoc filesys.hpp
 *
 */
namespace utl::filesys {

/**
 * \defgroup File File System
 * @ingroup Utils
 * @copydoc filesys.hpp
 * @{
 */
/**
 * @brief Converts a filesystem path to a UTF-8 string
 * @param path Filesystem path
 * @return UTF-8 encoded string
 */
std::string u8string(const std::filesystem::path& path);
/**
 * @brief Creates a filesystem path from a UTF-8 string
 * @param path UTF-8 string
 * @return Filesystem path
 */
std::filesystem::path u8path(std::string_view path);

/**
 * @brief Returns a new path with the specified file extension
 * @param path Original file path
 * @param new_extension New extension
 * @return A new path with replaced extension
 */
std::filesystem::path ReplaceExtension(const std::filesystem::path& path, const std::filesystem::path& new_extension);
/** @} */
}  // namespace utl::filesys

}  // namespace nnl
