/**
 * @file text.hpp
 * @brief Contains structures and functions for working with in-game text
 * archives.
 *
 * @see nnl::text::Text
 * @see nnl::text::IsOfType
 * @see nnl::text::Import
 * @see nnl::text::Export
 * @see nnl::text::Convert
 *
 */

#pragma once

#include <unordered_map>

#include "NNL/common/fixed_type.hpp"
#include "NNL/common/io.hpp"
#include "NNL/simple_asset/stexture.hpp"
#include "NNL/utility/data.hpp"
namespace nnl {
/**
 * \defgroup Text Text
 * @ingroup Visual
 * @copydoc text.hpp
 * @{
 */

/**
 * @copydoc text.hpp
 *
 */
namespace text {
/**
 * \defgroup Text_Main Main
 * @ingroup Text
 * @{
 */

/**
 * @brief A bitmask for identifying special codes within an IndexedString.
 *
 * Values where the upper 4 bits match this mask are interpreted as
 * formatting, control, or special characters rather than character indices.
 *
 * @see nnl::text::IndexedString
 */
constexpr u16 kSpecialCodeMask = 0xF000;

/**
 * @brief A default mapping of special codes to string representations.
 *
 * This map associates special codes that are used within indexed strings with their possible
 * textual representations. These codes represent symbols, special or control characters in the text.
 *
 * @see nnl::text::IndexedString
 * @see nnl::text::Convert
 */
const std::unordered_map<u16, std::string_view> kSpecialCodeToString{
    {0x1000, "{TRIANGLE}"},
    {0x1001, "{CIRCLE}"},
    {0x1002, "{X}"},
    {0x1003, "{SQUARE}"},
    {0x1004, "{UP}"},          // D-pad up
    {0x1005, "{DOWN}"},        // D-pad down
    {0x1006, "{LEFT}"},        // D-pad left
    {0x1007, "{RIGHT}"},       // D-pad right
    {0x1008, "{L}"},           // L
    {0x1009, ""},              // second half of L
    {0x100A, "{R}"},           // R
    {0x100B, ""},              // second half of R
    {0x100C, "{SELECT}"},      // select
    {0x100D, ""},              // second half of select
    {0x100E, "{START}"},       // start
    {0x100F, ""},              // second half of start
    {0x1010, "{JOYSTICK}"},    // joystick
    {0x1011, "{UNDERSCORE}"},  // underscore
    {0x1012, "{HEART}"},       // heart
    {0x1013, "{NOTE}"},        // note
    {0x1014, "{HEART1}"},      // slim heart
    {0x1015, "{NOTE1}"},       // slim note
    {0x1016, "{STAR}"},        // star

    {0x8000, "\0"},  // string terminator
    {0x8001, "\n"},  // new line
    {0x8002, "{8002}"},
    {0x8003, "{8003}"},
    {0x8004, "{8004}"},
    {0x8005, "{SMALL}"},     // small text size
    {0x8006, "{NORMAL}"},    // normal text size
    {0x8007, "{LARGE}"},     // large text size
    {0x8008, "{WHITE}"},     // reset/white
    {0x800A, "{BLACK}"},     // black text
    {0x800B, "{RED}"},       // red text
    {0x800C, "{BLUE}"},      // blue text
    {0x800D, "{ORANGE}"},    // orange text
    {0x800E, "{GREEN}"},     // green text
    {0x800F, "{800F}"},      //
    {0x8010, "{8010}"},      //
    {0x8011, "{8011}"},      //
    {0x8012, "{8012}"},      //
    {0x8013, "{8013}"},      //
    {0x8014, "{8014}"},      //
    {0x8015, "{8015}"},      //
    {0x8016, "{8016}"},      //
    {0x8017, "{8017}"},      //
    {0x8018, "{8018}"},      //
    {0x8019, "{8019}"},      //
    {0x801A, "{801A}"},      //
    {0x801B, "{801B}"},      //
    {0x801C, "{801C}"},      //
    {0x801D, "{801D}"},      // used 1 time in Spanish (tutorial)
    {0x801E, "{801E}"},      //
    {0x801F, "{801F}"},      //
    {0x8020, "{8020}"},      //
    {0x8021, "{8021}"},      //
    {0x8022, "{CYAN}"},      // cyan text
    {0x8023, "{MAGENTA}"},   // magenta text
    {0x8031, "\t"},          //
    {0x8032, "{EWHITE}"},    // Encircles a character with a white copy of itself
    {0x8033, "{EBLACK}"},    //
    {0x8034, "{EBLUE}"},     //
    {0x8035, "{EMAGENTA}"},  //
    {0x8036, "{ERED}"},      //
    {0x8037, " "},           // space
    {0x8038, "{VAR0}"},
    {0x8039, "{VAR1}"},

    {0x803A, "{803A}"},  //
    {0x803B, "{803B}"},  //
    {0x803C, "{803C}"},  //
    {0x803D, "{803D}"},  //
    {0x803E, "{803E}"},  //
    {0x803F, "{803F}"},  //

    {0x8040, "{VAR2}"},  // variable
    {0x8041, "{VAR3}"},  // variable
    {0x8042, "{VAR4}"},  // variable
    {0x8043, "{VAR5}"},  // small text size //variable ???
    {0x8044, "{VAR6}"},  // variable ???
    {0x8045, "{VAR7}"},  // variable
    {0x8046, "{VAR8}"},  // variable
    {0x8047, "{VAR9}"},  // variable
    {0x8048, "{VARA}"},  // small text size //variable ???
    {0x8049, "{VARB}"},  // variable ???
    {0x804A, "{804A}"},  // variable ???
    {0x804B, "{804B}"},  // variable ???
    {0x804C, "{804C}"},  // variable ???
    {0x804D, "{804D}"},  // variable ???
    {0x804E, "{804E}"},  // variable ???
    {0x804F, "{804F}"},  // variable ???
    {0x8050, "{VARC}"},  // variable ???
    {0x8051, "{VARD}"},
    {0x8052, "{VARE}"},
    {0x8053, "{VARF}"},
    // 1F4BD
};

/**
 * @brief A string consisting of special codes and indices into the
 * character lookup table.
 *
 * @see nnl::text::Text
 * @see nnl::text::kSpecialCodeMask
 * @see nnl::text::kSpecialCodeToString
 */
using IndexedString = std::vector<u16>;
/**
 * @brief Represents an in-game text archive.
 *
 * This structure represents an in-game text archive that consists of strings.
 * These strings may be used in dialogs, menus, and in other UI elements.
 *
 * @note This struct represents only a portion of the data required by the games to render
 * text. Its binary representation should be used as part of larger containers
 * (which differ between the games) that also include bitmap fonts and
 * glyph advance widths.
 *
 * @see nnl::asset::Asset
 * @see nnl::asset::BitmapText
 * @see nnl::texture::TextureContainer
 */
struct Text {
  std::vector<IndexedString>
      strings;  ///< Strings consisting of special codes and indices that reference the characters array.
  std::vector<u16> characters;  ///< A palette of unique UCS-2 encoded characters used by the strings.
};

/**
 * @brief Converts an in-game text archive to a vector of UTF-8 encoded strings.
 *
 * This function takes a Text archive and converts it into a vector of UTF-8
 * strings. It can also apply optional replacements for special codes using
 * the provided mapping.
 *
 * @param text The Text object to be converted.
 * @param replacements An optional map that provides replacements for
 *        special codes with their string representations.
 * @return A vector of strings representing the converted UTF-8 text.
 *
 * @see nnl::text::kSpecialCodeToString
 */
std::vector<std::string> Convert(const Text& text,
                                 const std::unordered_map<u16, std::string_view>& replacements = kSpecialCodeToString);
/**
 * @brief Converts a vector of UTF-8 encoded strings to an in-game text archive.
 *
 * This function takes a vector of UTF-8 strings and converts them into Text.
 *
 * @param stext A vector of UTF-8 encoded strings to be converted.
 * @param replacements An optional map that provides replacements for
 *        certain substrings with their code representations.
 * @param characters Precomputed UCS-2 character set. If provided, used as
 *        the character "palette" instead of generating one from the input text.
 * @return A Text object representing the converted in-game text archive.
 *
 * @see nnl::text::Text
 */
Text Convert(const std::vector<std::string>& stext,
             const std::unordered_map<u16, std::string_view>& replacements = kSpecialCodeToString,
             const std::vector<u16>& characters = {});

/**
 * @brief Tests if the provided file is a text archive.
 *
 * This function takes data representing a file and checks
 * whether it corresponds to the in-game text archive format.
 *
 * @param buffer The binary file to be tested.
 * @return Returns true if the file is identified as a text archive.
 *
 * @see nnl::text::Import
 */
bool IsOfType(BufferView buffer);

bool IsOfType(const std::filesystem::path& path);

bool IsOfType(Reader& f);

/**
 * @brief Parses a binary file and converts it to a Text object.
 *
 * This function takes a binary representation of a text archive,
 * parses its contents, and converts them into a `Text` struct for easier
 * access and manipulation.
 *
 * @param buffer The binary data to be processed.
 * @return A `Text` object representing the converted data.
 *
 * @see nnl::text::IsOfType
 * @see nnl::text::Export
 */
Text Import(BufferView buffer);

Text Import(const std::filesystem::path& path);

Text Import(Reader& f);
/**
 * @brief Converts a text archive to a binary file representation.
 *
 * This function takes a `Text` object and converts it into a Buffer,
 * which represents the binary format of the text.
 *
 * @param text The `Text` object to be converted into a binary format.
 * @return A `Buffer` containing the binary representation of the text.
 */
[[nodiscard]] Buffer Export(const Text& text);

void Export(const Text& text, const std::filesystem::path& path);

void Export(const Text& text, Writer& f);
/** @} */

/**
 * \defgroup Text_Auxiliary Auxiliary
 * @ingroup Text
 * @{
 */
/**
 * @brief Represents a bitmap font consisting of glyph atlases and advance widths.
 *
 * @note This struct is auxiliary and should be used to create other parts of a
 * complete text archive.
 *
 * @see nnl::text::GenerateBitmapFont
 * @see nnl::text::Text
 * @see nnl::asset::BitmapText
 */
struct BitmapFont {
  std::vector<STexture> bitmaps;  ///< Glyph atlases. @see nnl::texture::TextureContainer
  std::vector<u8> advance_width;  ///< Advance width for every character glyph. Each entry corresponds
                                  ///< to a character defined in the Text struct.
};
/**
 * @brief Parameters for generating a bitmap font.
 *
 * @see nnl::text::GenerateBitmapFont
 */
struct BitmapFontParams {
  unsigned int size = 128;          ///< The width and height of the bitmap. @note The games may need
                                    ///< to be patched to use bigger values.
  unsigned int columns = 8;         ///< Number of glyphs in a row of the bitmap
  float opacity_factor = 1.5f;      ///< A multiplier that makes glyphs brighter or dimmer
  float scale_factor = 1.0f;        ///< A multiplier that forcibly scales glyphs.
  int spacing_offset = 0;           ///< An additional offset for character spacing
  bool simulate_kerning = false;    ///< Simulates kerning between characters
                                    ///< (by using more characters)
  bool filter_nearest = false;      ///< Flag to use the nearest/linear neighbor filtering
  unsigned int alpha_levels = 256;  ///< Number of shades to use.
};
/**
 * @brief Generates a bitmap font from a TrueType font file.
 *
 * This function creates a BitmapFont object based on the provided text and
 * TrueType font, using the specified parameters for customization.
 *
 *
 * @param text The Text object containing the character data for the font (The object
 * may be changed if simulate_kerning is true)
 * @param ttf_font_path The path to the TrueType font file.
 * @param params An object containing customization options.
 * @return A BitmapFont object representing the generated bitmap font.
 * @see nnl::text::BitmapFont
 */
BitmapFont GenerateBitmapFont(Text& text, const std::filesystem::path& ttf_font_path,
                              const BitmapFontParams& params = {});

BitmapFont GenerateBitmapFont(Text& text, BufferView ttf_font_file, const BitmapFontParams& params = {});

/**
 * @brief Generates a BMFont .fnt file.
 *
 * This function creates a BMFont .fnt file using the provided data. The
 * generated .fnt file can be utilized in Godot.
 *
 * @param text The Text object containing the character data to be included in
 * the .fnt file.
 * @param advance_width Advance width for
 * each character.
 * @param bitmaps Character glyph bitmaps.
 * @param columns An optional parameter specifying the number of columns in the
 * bitmap. If not provided, it's calculated automatically.
 * @return A string containing the contents of the generated .fnt file.
 *
 * @see https://docs.godotengine.org/en/stable/tutorials/ui/gui_using_fonts.html#bitmap-fonts
 */
std::string GenerateFNT(const Text& text, const std::vector<u8>& advance_width, const std::vector<STexture>& bitmaps,
                        int columns = -1);
/** @} */

namespace raw {
/**
 * \defgroup Text_Raw Raw
 * @ingroup Text
 * @{
 */
constexpr u32 kMagicBytes = utl::data::FourCC("CDL!");  ///< Magic Bytes

struct RHeader {
  u32 num_strings = 0;
  std::vector<u32> offset_strings;
  u32 offset_cdl = 0;
  std::map<u32, std::vector<u16>> strings;
};

struct RCDL {
  u32 magic_bytes = kMagicBytes;
  u32 num_characters = 0;
  u32 num_0 = 0;  // reserved(?)
  u32 num_1 = 0;  // reserved(?)
  std::vector<u16> characters;
};

struct RText {
  RHeader header;
  RCDL cdl;
};

Text Convert(RText&& rtext);

RText Parse(Reader& f);
/** @} */
}  // namespace raw

}  // namespace text
/** @} */
}  // namespace nnl
