/**
 * @file ui.hpp
 *
 * @brief Contains structures and functions for working with in-game
 * ui configs.
 *
 * These configs define the layout, scaling, and
 * animation sequences (?) of UI elements.
 *
 * @note This module is a **draft**; currently, it only supports file type detection.
 *
 * @see nnl::ui::IsOfType
 * @see nnl::asset::Category
 */
#pragma once

#include "NNL/common/fixed_type.hpp"
#include "NNL/common/io.hpp"
namespace nnl {
/**
 * \defgroup UI UI Config
 * @ingroup Visual
 * @copydoc ui.hpp
 * @{
 */

/**
 * @copydoc ui.hpp
 *
 */
namespace ui {
/**
 * \defgroup UI_Main Main
 * @ingroup UI
 * @{
 */
/**
 * @brief Tests if the provided file is a UI config.
 *
 * This function takes data representing a file and checks
 * whether it corresponds to the in-game UI config format.
 *
 * @param buffer The file to be tested.
 * @return Returns true if the file is identified as a UI config;
 */
bool IsOfType(BufferView buffer);

bool IsOfType(const std::filesystem::path& path);

bool IsOfType(Reader& f);
/** @} */
namespace raw {
constexpr u32 kMagicBytes = 0x00'00'80'01;

NNL_PACK(struct RHeader {
  u32 magic_bytes = kMagicBytes;
  u16 num_struct_0 = 0;
  u16 num_struct_1 = 0;
  u16 num_struct_2 = 0;
  u16 num_struct_3 = 0;
  u16 num_struct_4 = 0;
  u16 num_struct_5 = 0;
  u16 num_struct_6 = 0;
  u16 num_struct_7 = 0;
  u16 num_struct_8 = 0;
  u16 padding = 0;
  u32 offset_struct_0 = 0;
  u32 offset_struct_1 = 0;
  u32 offset_struct_2 = 0;
  u32 offset_struct_3 = 0;
  u32 offset_struct_4 = 0;
  u32 offset_struct_5 = 0;
  u32 offset_struct_6 = 0;
  // u32 offset_struct_7 = 0;
  // u32 offset_struct_8 = 0;
});

static_assert(sizeof(RHeader) == 0x34);
}  // namespace raw

}  // namespace ui
/** @} */
}  // namespace nnl
