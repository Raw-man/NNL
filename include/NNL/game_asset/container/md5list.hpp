/**
 * @file md5list.hpp
 * @brief Provides structures and functions to manage .md5 checksum files
 * storing hashes for dig archive entries.
 *
 * @see nnl::md5list::Generate
 * @see nnl::md5list::Import
 * @see nnl::md5list::Export
 * @see nnl::dig::Dig
 */

#pragma once

#include <array>
#include <vector>

#include "NNL/common/io.hpp"
#include "NNL/game_asset/container/dig.hpp"

namespace nnl {

/**
 * \defgroup MD5List MD5 Checksum List
 * @ingroup Containers
 * @copydoc md5list.hpp
 *
 * @{
 */
/**
 * @copydoc md5list.hpp
 *
 */
namespace md5list {
/**
 * \defgroup MD5List_Main Main
 * @ingroup  MD5List
 * @{
 */

/**
 * @brief Generate MD5 checksums for every entry in a dig archive.
 * @param cfc_dig The dig archive whose entries (buffers) will be hashed.
 * @return A vector of MD5 digests
 *
 * @see nnl::md5list::Export
 * @see nnl::dig::Dig
 * @see nnl::dig::FileRecord
 */
std::vector<std::array<u8, 16>> Generate(const dig::Dig& cfc_dig);

std::vector<std::array<u8, 16>> Generate(const dig::DigView& cfc_dig);
/**
 * @brief Parses an .md5 checksum file into a vector of MD5 digests.
 * @param buffer The binary data to be processed.
 * @return A vector of MD5 digest for each entry of a corresponding dig archive.
 *
 * @see nnl::md5list::Export
 * @see nnl::md5list::Generate
 * @see nnl::utl::data::MD5
 */
std::vector<std::array<u8, 16>> Import(BufferView buffer);

std::vector<std::array<u8, 16>> Import(const std::filesystem::path& path);

std::vector<std::array<u8, 16>> Import(Reader& f);

/**
 * @brief Serialize a vector of MD5 digests into an .md5 checksum file format.
 * @param md5list A vector of MD5 checksums
 * @return A `Buffer` containing the binary representation of the checksum list.
 *
 * @see nnl::md5list::Generate
 * @see nnl::dig::Dig
 * @see nnl::dig::FileRecord
 * @see nnl::utl::data::MD5
 */
[[nodiscard]] Buffer Export(const std::vector<std::array<u8, 16>>& md5list);

void Export(const std::vector<std::array<u8, 16>>& md5list, const std::filesystem::path& path);

void Export(const std::vector<std::array<u8, 16>>& md5list, Writer& f);
/** @} */
}  // namespace md5list

/** @} */
}  // namespace nnl
