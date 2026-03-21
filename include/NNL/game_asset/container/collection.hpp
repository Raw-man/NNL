/**
 * @file collection.hpp
 * @brief Provides structures and functions to manage containers of distinct
 * but related assets.
 *
 * @see nnl::collection::Collection
 * @see nnl::collection::IsOfType
 * @see nnl::collection::ImportView
 * @see nnl::collection::Import
 * @see nnl::collection::Export
 */
#pragma once

#include <map>

#include "NNL/common/fixed_type.hpp"
#include "NNL/common/io.hpp"
namespace nnl {
/**
 * \defgroup Collection Collection
 * @ingroup Containers
 * @copydoc collection.hpp
 * @{
 */

/**
 * @copydoc collection.hpp
 *
 */
namespace collection {
/**
 * \defgroup Collection_Main Main
 * @ingroup Collection
 * @{
 */

/**
 * @brief A container for related but distinct assets.
 *
 * This container often groups complete, logically related but distinct assets (e.g., a main 3D map and its 3D props).
 * It's typically nested inside a DigEntry container.
 *
 * @see nnl::collection::Import
 * @see nnl::collection::Export
 * @see nnl::dig_entry::DigEntry
 * @see nnl::collection::CollectionView
 */
using Collection = std::vector<Buffer>;
/**
 * @brief A non-owning view of an asset collection.
 *
 * This type is almost identical to Collection and represents the same container but allows for access to its
 * entries via _references_ to the source Buffer. It's most suitable when no modifications to the data are
 * expected.
 *
 * @see nnl::collection::Collection
 * @see nnl::collection::ImportView
 */
using CollectionView = std::vector<BufferView>;

/**
 * @brief Tests if the provided file is an asset collection.
 *
 * This function takes data representing a file and checks
 * whether it corresponds to the in-game asset collection format.
 *
 * @param buffer The data to be tested.
 * @return true if the file is identified as an asset collection
 *
 * @see nnl::collection::Import
 * @see nnl::collection::ImportView
 */
bool IsOfType(BufferView buffer);

bool IsOfType(const std::filesystem::path& path);

bool IsOfType(Reader& f);

/**
 * @brief Parses a binary file and converts it into a Collection object.
 *
 * This function takes a binary representation of an asset collection,
 * parses its contents, and converts them into a Collection object for easier
 * access and modification.
 *
 * @param buffer The binary data to be processed.
 * @return A `Collection` object consisting of file buffers.
 *
 * @see nnl::collection::IsOfType
 * @see nnl::collection::ImportView
 * @see nnl::collection::Export
 */
Collection Import(BufferView buffer);

Collection Import(const std::filesystem::path& path);

Collection Import(Reader& f);
/**
 * @brief Parses a binary file and converts it into a CollectionView object.
 *
 * CollectionView does not store copies of
 * file buffers but stores _references_ to the original data.
 *
 * @param buffer The binary data to be processed.
 * @return A `CollectionView` object representing the converted data.
 *
 * @see nnl::collection::Collection
 */

CollectionView ImportView(BufferView buffer);

/**
 * @brief Converts an asset collection to a binary file representation.
 *
 * This function takes a Collection object and converts it into a Buffer,
 * which represents the binary format of the container.
 *
 * @param asset_collection The `Collection` object to be converted.
 * @return A `Buffer` containing the binary representation of the container.
 */
[[nodiscard]] Buffer Export(const Collection& asset_collection);

void Export(const Collection& asset_collection, const std::filesystem::path& path);

void Export(const Collection& asset_collection, Writer& f);

[[nodiscard]] Buffer Export(const CollectionView& asset_collection);

void Export(const CollectionView& asset_collection, const std::filesystem::path& path);

void Export(const CollectionView& asset_collection, Writer& f);
/** @} */
namespace raw {

NNL_PACK(struct RFileRecord { u32 offset = 0; });

static_assert(sizeof(RFileRecord) == 0x4);

template <typename TData = Buffer>
struct RCollection {
  u32 num_entries = 0;
  std::vector<RFileRecord> file_records;
  // Maps of offsets (used in the structures above) to their data:
  std::map<u32, TData> file_buffers;
};

RCollection<Buffer> Parse(Reader& f);

RCollection<BufferView> ParseView(BufferView f);

Collection Convert(RCollection<Buffer>&& rasset_collection);

CollectionView Convert(RCollection<BufferView>&& rasset_collection);

}  // namespace raw

}  // namespace collection

/** @} */
}  // namespace nnl
