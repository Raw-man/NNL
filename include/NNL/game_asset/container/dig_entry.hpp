/**
 * @file dig_entry.hpp
 * @brief Provides structures and functions to manage _entries_ of a dig archive.
 *
 * @see nnl::dig_entry::DigEntry
 * @see nnl::dig_entry::IsOfType
 * @see nnl::dig_entry::ImportView
 * @see nnl::dig_entry::Import
 * @see nnl::dig_entry::Export
 */
#pragma once

#include <map>
#include <vector>

#include "NNL/common/fixed_type.hpp"
#include "NNL/common/io.hpp"
namespace nnl {
/**
 * \defgroup DigEntry Dig Entry
 * @ingroup Containers
 * @copydoc dig_entry.hpp
 *
 * @{
 */

/**
 * @copydoc dig_entry.hpp
 *
 */
namespace dig_entry {

/**
 * \defgroup DigEntry_Main Main
 * @ingroup DigEntry
 * @{
 */

/**
 * @brief A nested archive within a top-level Dig archive.
 *
 *
 * @see nnl::dig_entry::Import
 * @see nnl::dig_entry::Export
 * @see nnl::dig_entry::DigEntryView
 * @see nnl::dig::FileRecord
 */
using DigEntry = std::vector<Buffer>;
/**
 * @brief A non-owning view of a dig entry archive.
 *
 * This type is almost identical to DigEntry and represents the same archive but allows for access to its entries
 * via _references_ to the source Buffer. It's most suitable when no
 * modifications to the data are expected.
 *
 * @see nnl::dig_entry::DigEntry
 * @see nnl::dig_entry::ImportView
 */
using DigEntryView = std::vector<BufferView>;
/**
 * @brief Tests if the provided file is a dig entry archive.
 *
 * This function takes data representing a file and checks
 * whether it corresponds to the in-game dig entry archive format.
 *
 * @param buffer The data to be tested.
 * @return true if the file is identified as a dig entry archive.
 *
 * @note The entries of a dig archive may be compressed and require decompression before they can be accessed.
 *
 * @see nnl::dig::Decompress
 * @see nnl::dig_entry::Import
 * @see nnl::dig_entry::ImportView
 */
bool IsOfType(BufferView buffer);

bool IsOfType(const std::filesystem::path& path);

bool IsOfType(Reader& f);
/**
 * @brief Parses a binary file and converts it into a DigEntry object.
 *
 * This function takes a binary representation of a dig entry archive,
 * parses its contents, and converts them into a DigEntry object for easier
 * access and modification.
 *
 * @param buffer The binary data to be processed.
 * @return A `DigEntry` object.
 *
 * @note The entries of a dig archive may be compressed and require decompression before they can be imported.
 *
 * @see nnl::dig::Decompress
 * @see nnl::dig_entry::IsOfType
 * @see nnl::dig_entry::ImportView
 * @see nnl::dig_entry::Export
 */
DigEntry Import(BufferView buffer);

DigEntry Import(const std::filesystem::path& path);

DigEntry Import(Reader& f);
/**
 * @brief Parses a binary file and converts it into DigEntryView.
 *
 * DigEntryView does not store copies of
 * file buffers but stores _references_ to the original data.
 *
 * @param buffer The binary data to be processed.
 * @return An `DigEntryView` object representing the converted data.
 *
 * @note The entries of a dig archive may be compressed and require decompression before they can be imported.
 *
 * @see nnl::dig::Decompress
 * @see nnl::dig_entry::IsOfType
 * @see nnl::dig_entry::Export
 */
DigEntryView ImportView(BufferView buffer);

/**
 * @brief Converts a dig entry archive to a binary file representation.
 *
 * This function takes an dig entry archive and converts it into a Buffer,
 * which represents the binary format of the container.
 *
 * @param dig_entry The object to be converted.
 * @return A `Buffer` containing the binary representation of the container.
 */
[[nodiscard]] Buffer Export(const DigEntry& dig_entry);

void Export(const DigEntry& dig_entry, const std::filesystem::path& path);

void Export(const DigEntry& dig_entry, Writer& f);

[[nodiscard]] Buffer Export(const DigEntryView& dig_entry);

void Export(const DigEntryView& dig_entry, const std::filesystem::path& path);

void Export(const DigEntryView& dig_entry, Writer& f);
/** @} */

namespace raw {
NNL_PACK(struct RFileRecord {
  u32 id = 0;
  u32 size = 0;
  u32 offset = 0;
  u32 reserved = 0;  // always 0
});

static_assert(sizeof(RFileRecord) == 0x10, "");

template <typename TData = Buffer>
struct RDigEntry {
  std::vector<RFileRecord> file_records;

  // Maps of offsets (used in the structures above) to their data:
  std::map<u32, TData> file_buffers;
};

RDigEntry<Buffer> Parse(Reader& f);

RDigEntry<BufferView> ParseView(BufferView f);

DigEntry Convert(RDigEntry<Buffer>&& cfcdig);

DigEntryView Convert(RDigEntry<BufferView>&& cfcdig);

}  // namespace raw
}  // namespace dig_entry
/** @} */
}  // namespace nnl
