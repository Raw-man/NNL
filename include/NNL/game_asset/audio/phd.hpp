/**
 * @file phd.hpp
 * @brief Contains a few functions and structures for working with PHD/PBD
 * sound banks, a proprietary format developed by Sony Computer Entertainment.
 *
 * @note This module is a **draft**; currently, it only supports file type validation.
 * More details can be found in the official PSP SDK documentation.
 *
 * @see nnl::phd::IsOfType
 * @see nnl::asset::SoundBank
 * @see nnl::asset::Category::kSoundBank
 */
#pragma once
#include "NNL/common/fixed_type.hpp"
#include "NNL/common/io.hpp"
#include "NNL/utility/data.hpp"
namespace nnl {
/**
 * \defgroup PHDPBD PHD/PBD Sound Banks
 * @ingroup Audio
 * @copydoc phd.hpp
 * @{
 */

/**
 * @copydoc phd.hpp
 *
 */
namespace phd {
/**
 * \defgroup PHDPBD_Main Main
 * @ingroup PHDPBD
 * @{
 */

/**
 * @brief Tests if the provided file is PHD/PBD sound bank.
 *
 * This function takes data representing a file and checks
 * whether it corresponds to the in-game sound bank format.
 *
 * @param buffer The file to be tested.
 * @return Returns true if the file is identified as a sound bank;
 */
bool IsOfType(BufferView buffer);

bool IsOfType(const std::filesystem::path& path);

bool IsOfType(Reader& f);

/** @} */

namespace raw {
/**
 * \defgroup PHDPBD_Raw Raw
 * @ingroup PHDPBD
 * @{
 */
constexpr u32 kMagicBytes = utl::data::FourCC("PPHD");  ///< Magic bytes

// PBD Header
NNL_PACK(struct RCommonAttr {
  u32 id = kMagicBytes;
  u32 attr_size = sizeof(RCommonAttr) - sizeof(u32) * 2;
  u32 version = 0x10000;
  u32 reserve1 = -1;
  u32 program_attr_offset = sizeof(RCommonAttr);
  u32 tone_attr_offset = sizeof(RCommonAttr);
  u32 vag_attr_offset = sizeof(RCommonAttr);
  i32 reserved[9] = {-1, -1, -1, -1, -1, -1, -1, -1, -1};
});

static_assert(sizeof(RCommonAttr) == 0x40);

NNL_PACK(struct RVagParam {
  u32 offset = -1;
  u32 sample_rate = -1;
  u32 size = -1;
  u32 reserved = -1;
});

static_assert(sizeof(RVagParam) == 0x10);

/** @} */
}  // namespace raw
}  // namespace phd
/** @} */
}  // namespace nnl
