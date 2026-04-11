/**
 * @file dig.hpp
 * @brief Provides structures and functions to manage primary game archives.
 *
 *
 * @see nnl::dig::Dig
 * @see nnl::dig::IsOfType
 * @see nnl::dig::ImportView
 * @see nnl::dig::Import
 * @see nnl::dig::Export
 */
#pragma once

#include <map>
#include <vector>

#include "NNL/common/fixed_type.hpp"
#include "NNL/common/io.hpp"

namespace nnl {

/**
 * \defgroup Dig Dig
 * @ingroup Containers
 * @copydoc dig.hpp
 *
 * @{
 */

/**
 * @copydoc dig.hpp
 *
 */
namespace dig {
/**
 * \defgroup Dig_Main Main
 * @ingroup  Dig
 * @{
 */

/**
 * @brief A raw entry in the dig archive.
 *
 * This structure encapsulates data and metadata of an entry in the dig archive.
 *
 * @note The buffer it stores is also a nested archive that may need to be decompressed before importing.
 *
 * @see nnl::dig::Dig
 * @see nnl::dig::Decompress
 *
 */
template <typename TData = Buffer>
struct TFileRecord {
  bool is_compressed = false;  ///< If the buffer is compressed
  u32 decompressed_size = 0;   ///< Expected size after decompression. Matches the size of the buffer
                               ///< if it's not compressed.
  u16 num_entries = 0;         ///< The number of entries in the _nested_ archive.
  TData buffer;                ///< Binary file buffer that stores a nested archive. @see nnl::dig_entry::DigEntry
};

/**
 * @copybrief nnl::dig::TFileRecord
 *
 * @see nnl::dig::TFileRecord
 * @see nnl::dig::Dig
 */
using FileRecord = TFileRecord<Buffer>;

/**
 * @copybrief nnl::dig::TFileRecord
 *
 * @see nnl::dig::TFileRecord
 * @see nnl::dig::DigView
 */
using FileRecordView = TFileRecord<BufferView>;

/**
 * @brief A primary game data archive.
 *
 * The primary container for game assets. The archive format is known by different names:
 * CFC.DIG in @ref NUC2, *.BIN in NSUNI and NSLAR. It consists of nested archives.
 *
 * @see nnl::dig::Import
 * @see nnl::dig::Export
 * @see nnl::dig::FileRecord
 * @see nnl::dig::DigView
 * @see nnl::dig_entry::DigEntry
 * @see nnl::md5list::Generate
 */
using Dig = std::vector<FileRecord>;
/**
 * @brief A non-owning view of a dig archive.
 *
 * This type is almost identical to Dig and represents the same archive but allows for access to its entries
 * via _references_ to the source Buffer. It's most suitable when no
 * modifications to the data are expected.
 *
 * @see nnl::dig::Dig
 * @see nnl::dig::ImportView
 */
using DigView = std::vector<FileRecordView>;

/**
 * @brief Tests if the provided file is a dig archive.
 *
 * This function takes data representing a file and checks
 * whether it corresponds to the in-game dig archive format.
 *
 * @param buffer The data to be tested.
 * @return true if the file is identified as a dig archive
 *
 * @see nnl::dig::Import
 * @see nnl::dig::ImportView
 */
bool IsOfType(BufferView buffer);

bool IsOfType(const std::filesystem::path& path);

bool IsOfType(Reader& f);
/**
 * @brief Parses a binary file and converts it into a Dig object.
 *
 * This function takes a binary representation of a dig archive,
 * parses its contents, and converts them into a Dig object for easier
 * access and modification.
 *
 * @param buffer The binary data to be processed.
 * @return A `Dig` object consisting of file records.
 *
 * @note In NSUNI, the archives may be encrypted and must be
 *       decrypted prior to importing.
 *
 * @see nnl::format::kPGD
 * @see nnl::dig::IsOfType
 * @see nnl::dig::ImportView
 * @see nnl::dig::Export
 */
Dig Import(BufferView buffer);

Dig Import(const std::filesystem::path& path);

Dig Import(Reader& f);
/**
 * @brief Parses a binary file and converts it into DigView.
 *
 * DigView does not store copies of
 * file buffers but stores _references_ to the original data.
 *
 * @param buffer The binary data to be processed.
 * @return A `DigView` object representing the converted data.
 *
 * @note In NSUNI, the archives may be encrypted and must be
 *       decrypted prior to importing.
 *
 * @see nnl::format::kPGD
 * @see nnl::dig::Dig
 * @see nnl::dig::IsOfType
 * @see nnl::dig::Export
 *
 */
DigView ImportView(BufferView buffer);

/**
 * @brief Converts a dig archive to a binary file representation.
 *
 * This function takes a Dig archive and converts it into a Buffer,
 * which represents the binary format of the container.
 *
 * @param cfcdig The object to be converted.
 * @return A `Buffer` containing the binary representation of the container.
 */
[[nodiscard]] Buffer Export(const Dig& cfcdig);

void Export(const Dig& cfcdig, const std::filesystem::path& path);

void Export(const Dig& cfcdig, Writer& f);

[[nodiscard]] Buffer Export(const DigView& cfcdig);

void Export(const DigView& cfcdig, const std::filesystem::path& path);

void Export(const DigView& cfcdig, Writer& f);
/** @} */

/**
 * \defgroup Dig_Aux Auxiliary
 * @ingroup Dig
 *
 * @{
 */

/**
 * @brief Decompress binary data
 *
 * This function can be used to decompress binary buffers in dig archives
 *
 * @param buffer Data to decompress
 * @param decompressed_size The expected size of decompressed data
 * @return Decompressed data
 *
 * @see nnl::dig::FileRecord
 * @see nnl::dig::Compress
 */
Buffer Decompress(BufferView buffer, u32 decompressed_size);
/**
 * @brief Compress binary data
 *
 * @param buffer Data to compress
 * @return Compressed data
 *
 * @see nnl::dig::Decompress
 */
Buffer Compress(BufferView buffer);
/** @} */

namespace raw {
/**
 * \defgroup Dig_Raw Raw
 * @ingroup Dig
 *
 * @{
 */
constexpr u32 kBlockSize = 0x800U;  ///< The size of an LBA sector in bytes

NNL_PACK(struct RFileRecord {
  u32 offset = 0;  // in blocks (*0x800)
  u32 compressed_size = 0;
  u16 num_entries = 0;
  u16 is_compressed = 0;
  u32 decompressed_size = 0;
});

static_assert(sizeof(RFileRecord) == 0x10);

template <typename TData = Buffer>
struct RDig {
  std::vector<RFileRecord> file_records;

  // Maps of offsets (used in the structures above) to their data:
  std::map<u32, TData> file_buffers;
};

RDig<Buffer> Parse(Reader& f);

RDig<BufferView> ParseView(BufferView f);

Dig Convert(RDig<Buffer>&& cfcdig);

DigView Convert(RDig<BufferView>&& cfcdig);
/** @} */
}  // namespace raw

}  // namespace dig
/** @} */
}  // namespace nnl
