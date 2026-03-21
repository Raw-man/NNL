/**
 * @file vertexde.hpp
 * @brief Contains functions that facilitate conversion of vertex
 * data.
 *
 * The `vertexde` namespace provides methods that enable conversion between a
 * simple higher-level vertex representation and binary vertex buffers
 * supported by the @ref GE.

 * @see nnl::vertexde::fmt
 * @see nnl::vertexde::Decode
 * @see nnl::vertexde::Encode
 * @see nnl::SVertex
 * @see nnl::model::Model
 */

#pragma once

#include "NNL/common/contract.hpp"
#include "NNL/common/fixed_type.hpp"
#include "NNL/common/io.hpp"
#include "NNL/simple_asset/smodel.hpp"
namespace nnl {
/**
 * \defgroup Vertex Vertex Decode/Encode Utilities
 * @ingroup Visual
 * @copydoc vertexde.hpp
 * @{
 */
/**
 * @copydoc vertexde.hpp
 *
 */
namespace vertexde {
/**
 * \defgroup Vertex_Auxiliary Auxiliary
 * @ingroup Vertex
 * @{
 */
/**
 * @namespace nnl::vertexde::fmt_code
 *
 * @brief This namespace defines constants that represent fundamental format
 * codes.
 *
 * @note The codes in this namespace are not final values; they must be shifted
 * to obtain the actual vertex format constants.
 *
 * @see nnl::vertexde::fmt
 * @see nnl::vertexde::fmt_shift
 */
namespace fmt_code {
constexpr u32 k0 = 0;  ///< no component

constexpr u32 k8 = 1;   ///< 1 byte component
constexpr u32 k16 = 2;  ///< 2 byte component
constexpr u32 k32 = 3;  ///< 4 byte component

constexpr u32 k565 = 4;   ///< 2 byte color
constexpr u32 k5551 = 5;  ///< 2 byte color
constexpr u32 k4444 = 6;  ///< 2 byte color
constexpr u32 k8888 = 7;  ///< 4 byte color
}  // namespace fmt_code
/**
 * @namespace nnl::vertexde::fmt_shift
 *
 * @brief This namespace contains constants representing shift values that are
 * applied to format codes to create the vertex format constants.
 *
 * @see nnl::vertexde::fmt
 * @see nnl::vertexde::fmt_code
 */
namespace fmt_shift {
constexpr u32 kUV = 0;

constexpr u32 kColor = 2;

constexpr u32 kNormal = 5;

constexpr u32 kPosition = 7;

constexpr u32 kWeight = 9;

constexpr u32 kIndex = 11;

constexpr u32 kWeightNum = 14;

constexpr u32 kMorphNum = 18;

constexpr u32 kThrough = 23;
}  // namespace fmt_shift
/**
 * @namespace nnl::vertexde::fmt_mask
 *
 * @brief This namespace contains constants representing bit masks for parts of
 * the vertex format.
 *
 * @see nnl::vertexde::fmt
 */
namespace fmt_mask {
constexpr u32 kUV = 0b0'00'000'0'000'0'00'00'00'00'000'11;

constexpr u32 kColor = 0b0'00'000'0'000'0'00'00'00'00'111'00;

constexpr u32 kNormal = 0b0'00'000'0'000'0'00'00'00'11'000'00;

constexpr u32 kPosition = 0b0'00'000'0'000'0'00'00'11'00'000'00;

constexpr u32 kWeight = 0b0'00'000'0'000'0'00'11'00'00'000'00;

constexpr u32 kIndex = 0b0'00'000'0'000'0'11'00'00'00'000'00;

constexpr u32 kWeightNum = 0b0'00'000'0'111'0'00'00'00'00'000'00;

constexpr u32 kMorphNum = 0b0'00'111'0'000'0'00'00'00'00'000'00;

constexpr u32 kThrough = 0b1'00'000'0'000'0'00'00'00'00'000'00;
}  // namespace fmt_mask
/** @} */

/**
 * \defgroup Vertex_Main Main
 * @ingroup Vertex
 * @{
 */

/**
 * @namespace nnl::vertexde::fmt
 *
 * @brief This namespace contains constants and functions for defining the
 * vertex format.
 *
 * The constants in this namespace represent various formats for vertex
 * attributes, and control the drawing modes.
 * These constants should be combined together using bitwise OR to obtain the final
 * vertex format value (which is stored in the lower 24 bits of a 32-bit variable).
 *
 * @see nnl::model::Model
 * @see nnl::model::GeCmd
 * @see nnl::utl::math::FloatToFixed
 *
 * @note Vertex positions must always be present in the
 * vertex format.
 */
namespace fmt {
/**
 * @brief 8-bit fixed-point texture coordinates (unsigned)
 */
constexpr u32 kUV8 = fmt_code::k8 << fmt_shift::kUV;
/**
 * @brief 16-bit fixed-point texture coordinates (unsigned)
 */
constexpr u32 kUV16 = fmt_code::k16 << fmt_shift::kUV;
/**
 * @brief floating-point texture coordinates
 */
constexpr u32 kUV32 = fmt_code::k32 << fmt_shift::kUV;
/**
 * @brief 2-byte color format.  No alpha channel.
 */
constexpr u32 kColor565 = fmt_code::k565 << fmt_shift::kColor;
/**
 * @brief 2-byte color format. 1 bit for the alpha channel.
 */
constexpr u32 kColor5551 = fmt_code::k5551 << fmt_shift::kColor;
/**
 * @brief 2-byte color format. 4 bits per channel.
 */
constexpr u32 kColor4444 = fmt_code::k4444 << fmt_shift::kColor;
/**
 * @brief 4-byte color format. 8 bits per channel.
 */
constexpr u32 kColor8888 = fmt_code::k8888 << fmt_shift::kColor;
/**
 * @brief 8-bit fixed-point vertex normal (signed).
 */
constexpr u32 kNormal8 = fmt_code::k8 << fmt_shift::kNormal;
/**
 * @brief 16-bit fixed-point vertex normal (signed).
 */
constexpr u32 kNormal16 = fmt_code::k16 << fmt_shift::kNormal;
/**
 * @brief floating-point vertex normal.
 */
constexpr u32 kNormal32 = fmt_code::k32 << fmt_shift::kNormal;
/**
 * @brief 8-bit fixed-point vertex coordinates (signed).
 */
constexpr u32 kPosition8 = fmt_code::k8 << fmt_shift::kPosition;
/**
 * @brief 16-bit fixed-point vertex coordinates (signed).
 */
constexpr u32 kPosition16 = fmt_code::k16 << fmt_shift::kPosition;
/**
 * @brief floating-point vertex coordinates.
 */
constexpr u32 kPosition32 = fmt_code::k32 << fmt_shift::kPosition;
/**
 * @brief 8-bit fixed-point bone weights (unsigned).
 * @see nnl::vertexde::fmt::kWeightNum
 */
constexpr u32 kWeight8 = fmt_code::k8 << fmt_shift::kWeight;
/**
 * @brief 16-bit fixed-point bone weights (unsigned).
 * @see nnl::vertexde::fmt::kWeightNum
 */
constexpr u32 kWeight16 = fmt_code::k16 << fmt_shift::kWeight;
/**
 * @brief floating-point bone weights.
 * @see nnl::vertexde::fmt::kWeightNum
 */
constexpr u32 kWeight32 = fmt_code::k32 << fmt_shift::kWeight;
/**
 * @brief unsigned 8-bit indices (in index buffers)
 *
 * This constant enables the use of index buffers for
 * more efficient storage.
 *
 * @note Indices are not part of vertex attributes
 */
constexpr u32 kIndex8 = fmt_code::k8 << fmt_shift::kIndex;
/**
 * @brief unsigned 16-bit indices (in index buffers)
 *
 * @copydetails kIndex8
 */
constexpr u32 kIndex16 = fmt_code::k16 << fmt_shift::kIndex;

// kIndex32: technically exists but unused

/**
 * @brief Indicates whether the 2D drawing mode is used.
 *
 * This constant enables the Through Mode, allowing vertices to bypass the
 * 3D transformation pipeline and be drawn **directly** to the screen.
 *
 * @note Only positions, texture coordinates, and vertex colors are supported in
 * this mode. These attributes should be encoded as 16-bit integers.
 * See the official PSP SDK docs for more details.
 *
 */
constexpr u32 kThrough = 1 << fmt_shift::kThrough;

/**
 * @brief Generates a constant that encodes the number of weights.
 *
 * This function takes a number and returns a value that
 * can be combined with the vertex format to specify the number of used weights.
 *
 * @param weight_num The number of weights (1-8).
 * @return A constant representing the specified number of weights.
 *
 * @note Ensure that weights are also enabled in the vertex format.
 *
 * @see nnl::vertexde::fmt::kWeight8
 * @see nnl::vertexde::fmt::kWeight16
 * @see nnl::vertexde::fmt::kWeight32
 */
constexpr inline u32 kWeightNum(u32 weight_num) {
  NNL_EXPECTS_DBG(weight_num > 0 && weight_num <= 8);
  return ((weight_num - 1) << fmt_shift::kWeightNum);
}

/**
 * @brief Generates a constant that encodes the number of morph targets.
 *
 * This function takes a number and returns a value that
 * can be combined with the vertex format to specify the number of morph
 * targets. A morph target is a set of various vertex attributes that represents
 * a different state of the same vertex, rather than a different vertex entirely.
 * There's always at least 1 "target".
 *
 * @param morph_num The number of morph targets (1-8).
 * @return A constant representing the specified number of morph targets.
 *
 * @note The games do **not** have an animation system that controls morph
 * targets. Essentially, morphing is not supported. However, since the PSP's
 * Graphics Engine does support it, it's technically possible to encode multiple
 * morph targets into vertex buffers (and insert additional commands into
 * display lists to render such meshes). Furthermore, in theory, it should
 * be possible to modify the games to enable support for these animations.
 */
constexpr inline u32 kMorphNum(u32 morph_num) {
  NNL_EXPECTS_DBG(morph_num > 0 && morph_num <= 8);
  return ((morph_num - 1) << fmt_shift::kMorphNum);
}
}  // namespace fmt

/**
 * @brief Extracts vertex data from a buffer and converts it to a simple
 * representation.
 *
 * This function processes the provided vertex buffer according to the specified
 * vertex format and extracts various vertex attributes from it.
 *
 * @param vertex_buffer The buffer containing the vertex data.
 * @param vertex_format The format of the vertex data in the buffer.
 * @param bone_indices An array of bone indices associated with the vertex
 * weights.
 * @return A vector of `SVertex`.
 *
 * @note All of the possible vertex attributes are present in the SVertex struct.
 * One must inspect the vertex_format to see which ones are actually used.
 *
 * @see nnl::vertexde::Encode
 * @see nnl::model::Primitive
 * @see nnl::model::SubMesh
 */
std::vector<SVertex> Decode(BufferView vertex_buffer, u32 vertex_format, std::array<u16, 8> bone_indices = {0});
/**
 * @brief Converts vertex data from a simple representation to a binary vertex
 * buffer.
 *
 * This function converts the provided vector of `SVertex` structures
 * into a binary buffer suitable for use by the @ref GE.
 *
 * @param vertices A vector of `SVertex` to convert.
 * @param vertex_format The format of the vertex data, which determines how the attributes are encoded.
 * @param bone_indices An array of bone indices associated with the vertex
 * weights.
 * @return A Buffer containing the serialized vertex data.
 *
 * @see nnl::vertexde::Decode
 * @see nnl::vertexde::fmt
 */
Buffer Encode(const std::vector<SVertex>& vertices, u32 vertex_format, std::array<u16, 8> bone_indices = {0});
/** @} */  // Main

/**
 * \addtogroup Vertex_Auxiliary
 * @ingroup Vertex
 * @{
 */

/**
 * @brief Maps format codes to attribute sizes
 *
 * @see nnl::vertexde::fmt_code
 */
constexpr std::array<u32, 8> format_sizes{0, 1, 2, 4, 2, 2, 2, 4};

// through
/**
 * @brief Retrieves the Through Mode status from the vertex format.
 *
 * @param vertex_format The vertex format from which to extract the Through Mode
 * status.
 *
 * @return A value indicating the status of the Through Mode (0 or 1).
 */
inline u32 GetThrough(u32 vertex_format) { return (vertex_format & fmt_mask::kThrough) >> fmt_shift::kThrough; }

// WEIGHTS
/**
 * @brief Retrieves the weight format code from the vertex format.
 *
 * @param vertex_format The vertex format from which to extract the weight
 * format.
 * @return A value representing the weight format code.
 */
inline u32 GetWeightFormat(u32 vertex_format) { return (vertex_format & fmt_mask::kWeight) >> fmt_shift::kWeight; }
/**
 * @brief Retrieves the byte size of a weight component from the vertex
 * format.
 *
 *
 * @param vertex_format The vertex format from which to extract the weight info.
 * @return The size of a single component in bytes.
 */
inline u32 GetWeightSize(u32 vertex_format) { return format_sizes.at(GetWeightFormat(vertex_format)); }
/**
 * @brief Retrieves the number of weight components from the vertex format.
 *
 * @param vertex_format The vertex format from which to extract the number of
 * weights.
 * @return The number of weights, which is a value between 0 and 8.
 */
inline u32 GetWeightNum(u32 vertex_format) {
  return GetWeightFormat(vertex_format) != 0 ? ((vertex_format & fmt_mask::kWeightNum) >> fmt_shift::kWeightNum) + 1
                                             : 0;
}

// UV
/**
 * @brief Retrieves the UV format code from the vertex format.
 *
 * @param vertex_format The vertex format from which to extract the UV format.
 * @return A value representing the UV format code.
 */
inline u32 GetUVFormat(u32 vertex_format) { return (vertex_format & fmt_mask::kUV) >> fmt_shift::kUV; }
/**
 * @brief Retrieves the byte size of a UV component from the vertex
 * format.
 *
 * @param vertex_format The vertex format from which to extract the UV info.
 * @return The size of a single component in bytes.
 */
inline u32 GetUVSize(u32 vertex_format) { return format_sizes.at(GetUVFormat(vertex_format)); }
/**
 * @brief Retrieves the number of UV components.
 *
 * @return The number of UV components, which is either 0 or 2.
 */
inline u32 GetUVNum(u32 vertex_format) { return GetUVFormat(vertex_format) != 0 ? 2 : 0; }

// COLOR
/**
 * @brief Retrieves the color format code from the vertex format.
 *
 * @param vertex_format The vertex format from which to extract the color
 * format.
 * @return A value representing the color format code.
 */
inline u32 GetColorFormat(u32 vertex_format) { return (vertex_format & fmt_mask::kColor) >> fmt_shift::kColor; }
/**
 * @brief Retrieves the byte size of a vertex color from the vertex format.
 *
 * @param vertex_format The vertex format from which to extract the info.
 * @return The size of a color in bytes (0, 2, or 4).
 */
inline u32 GetColorSize(u32 vertex_format) { return format_sizes.at(GetColorFormat(vertex_format)); }
/**
 * @brief Retrieves the number of colors.
 *
 * @return The number of colors, which is always 0 or 1.
 */
inline u32 GetColorNum(u32 vertex_format) { return GetColorFormat(vertex_format) != 0 ? 1 : 0; }

// NORMAL
/**
 * @brief Retrieves the normal format code from the vertex format.
 *
 * @param vertex_format The vertex format from which to extract the normal
 * format.
 * @return A value representing the normal format code.
 */
inline u32 GetNormalFormat(u32 vertex_format) { return (vertex_format & fmt_mask::kNormal) >> fmt_shift::kNormal; }
/**
 * @brief Retrieves the byte size of a normal component from the vertex
 * format.
 *
 * @param vertex_format The vertex format from which to extract the normal info.
 * @return The size of a single component in bytes.
 */
inline u32 GetNormalSize(u32 vertex_format) { return format_sizes.at(GetNormalFormat(vertex_format)); }
/**
 * @brief Retrieves the number of normal components from the vertex format.
 *
 * @param vertex_format The vertex format from which to extract the number of
 * normals.
 * @return The number of normal components, which is either 3 or 0.
 */
inline u32 GetNormalNum(u32 vertex_format) { return GetNormalFormat(vertex_format) != 0 ? 3 : 0; }

// POSITION
/**
 * @brief Retrieves the position format code from the vertex format.
 *
 * @param vertex_format The vertex format from which to extract the position
 * format.
 * @return A value representing the position format code.
 */
inline u32 GetPositionFormat(u32 vertex_format) {
  return (vertex_format & fmt_mask::kPosition) >> fmt_shift::kPosition;
}
/**
 * @brief Retrieves the byte size of a position component from the vertex
 * format.
 *
 * @param vertex_format The vertex format from which to extract the position
 * info.
 * @return The size of a single component in bytes, which is at least 1
 * for valid formats.
 */
inline u32 GetPositionSize(u32 vertex_format) { return format_sizes.at(GetPositionFormat(vertex_format)); }
/**
 * @brief Retrieves the number of position components.
 *
 * @param vertex_format The vertex format from which to extract the position
 * info.
 *
 * @return The number of position components, which is always 3 for valid
 * formats.
 */
inline u32 GetPositionNum(u32 vertex_format) { return GetPositionFormat(vertex_format) != 0 ? 3 : 0; }

// MORPHING
/**
 * @brief Retrieves the number of morph targets from the vertex format.
 *
 * @param vertex_format The vertex format from which to extract the number of
 * morph targets.
 * @return The number of morph targets, which is a value between 1 and 8.
 */
inline u32 GetMorphNum(u32 vertex_format) {
  return ((vertex_format & fmt_mask::kMorphNum) >> fmt_shift::kMorphNum) + 1;
}

// INDEX
/**
 * @brief Retrieves the index format code from the vertex format.
 *
 * @param vertex_format The vertex format from which to extract the index
 * format.
 * @return A value representing the index format code.
 *
 */
inline u32 GetIndexFormat(u32 vertex_format) { return (vertex_format & fmt_mask::kIndex) >> fmt_shift::kIndex; }
/**
 * @brief Retrieves the byte size of a single index from the vertex format.
 *
 * @param vertex_format The vertex format from which to extract the index info.
 * @return The size of an index in bytes.
 *
 */
inline u32 GetIndexSize(u32 vertex_format) { return format_sizes.at(GetIndexFormat(vertex_format)); }

// HAS
/**
 * @brief Checks if the vertex format includes weight information.
 *
 * @param vertex_format The vertex format to check for weight information.
 * @return True if the vertex format includes weights.
 */
inline bool HasWeights(u32 vertex_format) { return static_cast<bool>(GetWeightFormat(vertex_format)); }
/**
 * @brief Checks if the vertex format includes UV information.
 *
 * @param vertex_format The vertex format to check for UV information.
 * @return True if the vertex format includes UV.
 */
inline bool HasUV(u32 vertex_format) { return static_cast<bool>(GetUVFormat(vertex_format)); }
/**
 * @brief Checks if the vertex format includes color information.
 *
 *
 * @param vertex_format The vertex format to check for color information.
 * @return True if the vertex format includes color.
 */
inline bool HasColor(u32 vertex_format) { return static_cast<bool>(GetColorFormat(vertex_format)); }
/**
 * @brief Checks if the vertex format includes normal information.
 *
 *
 * @param vertex_format The vertex format to check for normal information.
 * @return True if the vertex format includes normals.
 */
inline bool HasNormal(u32 vertex_format) { return static_cast<bool>(GetNormalFormat(vertex_format)); }
/**
 * @brief Checks if the vertex format includes position information.
 *
 *
 * @param vertex_format The vertex format to check for position information.
 * @return True if the vertex format is valid and includes positions
 * @note A valid vertex format always must include positions.
 */
inline bool HasPosition(u32 vertex_format) { return static_cast<bool>(GetPositionFormat(vertex_format)); }

// IS
/**
 * @brief Checks if the vertex format uses indexed drawing.
 *
 * @param vertex_format The vertex format to check for index information.
 * @return True if the vertex format uses indexed drawing; otherwise, false.
 *
 *
 * @see nnl::vertexde::fmt::kIndex16
 */
inline bool IsIndexed(u32 vertex_format) { return static_cast<bool>(GetIndexFormat(vertex_format)); }

/**
 * @brief Checks if the vertex format uses Through Mode (2D drawing).
 *
 * @param vertex_format The vertex format to check for index information.
 * @return True if the vertex format uses indexed drawing; otherwise, false.
 *
 *
 * @see nnl::vertexde::fmt::kThrough
 */
inline bool IsThrough(u32 vertex_format) { return static_cast<bool>(GetThrough(vertex_format)); }

// INDICES
/**
 * @brief Checks if the vertex format uses 8-bit indices.
 *
 *
 * @param vertex_format The vertex format to check.
 * @return True if the vertex format uses 8-bit indices.
 */
inline bool Is8Indices(u32 vertex_format) {
  return (vertexde::GetIndexFormat(vertex_format) == vertexde::fmt_code::k8);
}
/**
 * @brief Checks if the vertex format uses 16-bit indices.
 *
 * @param vertex_format The vertex format to check.
 * @return True if the vertex format uses 16-bit indices.
 */
inline bool Is16Indices(u32 vertex_format) {
  return (vertexde::GetIndexFormat(vertex_format) == vertexde::fmt_code::k16);
}

// WEIGHTS
/**
 * @brief Checks if the vertex format uses 8-bit weights.
 *
 * This function determines whether the provided vertex format is configured
 * to use unsigned 8-bit fixed-point numbers for weights.
 *
 * @param vertex_format The vertex format to check.
 * @return True if the vertex format uses 8-bit weights.
 */
inline bool Is8Weights(u32 vertex_format) {
  return (vertexde::GetWeightFormat(vertex_format) == vertexde::fmt_code::k8);
}
/**
 * @brief Checks if the vertex format uses 16-bit weights.
 *
 * This function determines whether the provided vertex format is configured
 * to use unsigned 16-bit fixed-point numbers for weights.
 *
 * @param vertex_format The vertex format to check.
 * @return True if the vertex format uses 16-bit weights.
 */
inline bool Is16Weights(u32 vertex_format) {
  return (vertexde::GetWeightFormat(vertex_format) == vertexde::fmt_code::k16);
}

/**
 * @brief Checks if the vertex format uses float weights.
 *
 * This function determines whether the provided vertex format is configured
 * to use float numbers for weights.
 *
 * @param vertex_format The vertex format to check.
 * @return True if the vertex format uses float weights.
 */
inline bool Is32Weights(u32 vertex_format) {
  return (vertexde::GetWeightFormat(vertex_format) == vertexde::fmt_code::k32);
}
/**
 * @brief Checks if the vertex format uses fixed-point weights.
 *
 * This function determines whether the provided vertex format is configured
 * to use unsigned 8 or 16 bit fixed-point numbers for weights.
 *
 * @param vertex_format The vertex format to check.
 * @return True if the vertex format uses fixed-point weights (8 or 16 bits).
 */
inline bool IsFixedWeights(u32 vertex_format) { return (Is8Weights(vertex_format) || Is16Weights(vertex_format)); }

// UV
/**
 * @brief Checks if the vertex format uses 8-bit UVs.
 *
 * This function determines whether the provided vertex format is configured
 * to use unsigned 8-bit fixed-point numbers for UV components.
 *
 * @param vertex_format The vertex format to check.
 * @return True if the vertex format uses 8-bit UV components.
 */
inline bool Is8UV(u32 vertex_format) { return (vertexde::GetUVFormat(vertex_format) == vertexde::fmt_code::k8); }
/**
 * @brief Checks if the vertex format uses 16-bit UVs.
 *
 * This function determines whether the provided vertex format is configured
 * to use unsigned 16-bit fixed-point numbers for UV components.
 *
 * @param vertex_format The vertex format to check.
 * @return True if the vertex format uses 16-bit UV components.
 */
inline bool Is16UV(u32 vertex_format) { return (vertexde::GetUVFormat(vertex_format) == vertexde::fmt_code::k16); }
/**
 * @brief Checks if the vertex format uses float UVs.
 *
 * This function determines whether the provided vertex format is configured
 * to use float numbers for UVs.
 *
 * @param vertex_format The vertex format to check.
 * @return True if the vertex format uses float UV components.
 */
inline bool Is32UV(u32 vertex_format) { return (vertexde::GetUVFormat(vertex_format) == vertexde::fmt_code::k32); }
/**
 * @brief Checks if the vertex format uses fixed-point UV components.
 *
 * This function checks if the vertex format is configured
 * to use unsigned 8 or 16 bit fixed-point UV components.
 *
 * @param vertex_format The vertex format to check.
 * @return True if the vertex format uses 8 or 16 bit fixed-point UVs.
 */
inline bool IsFixedUV(u32 vertex_format) { return (Is8UV(vertex_format) || Is16UV(vertex_format)); }

// COLOR
/**
 * @brief Checks if the vertex format uses 565 vertex colors.
 *
 * @param vertex_format The vertex format to check.
 * @return True if the vertex format uses 565 vertex colors.
 */
inline bool Is565Color(u32 vertex_format) {
  return (vertexde::GetColorFormat(vertex_format) == vertexde::fmt_code::k565);
}
/**
 * @brief Checks if the vertex format uses 5551 vertex colors.
 *
 * @param vertex_format The vertex format to check.
 * @return True if the vertex format uses 5551 vertex colors.
 */
inline bool Is5551Color(u32 vertex_format) {
  return (vertexde::GetColorFormat(vertex_format) == vertexde::fmt_code::k5551);
}
/**
 * @brief Checks if the vertex format uses 4444 vertex colors.
 *
 * @param vertex_format The vertex format to check.
 * @return True if the vertex format uses 4444 vertex colors.
 */
inline bool Is4444Color(u32 vertex_format) {
  return (vertexde::GetColorFormat(vertex_format) == vertexde::fmt_code::k4444);
}
/**
 * @brief Checks if the vertex format uses 8888 vertex colors.
 *
 * @param vertex_format The vertex format to check.
 * @return True if the vertex format uses 8888 vertex colors.
 */
inline bool Is8888Color(u32 vertex_format) {
  return (vertexde::GetColorFormat(vertex_format) == vertexde::fmt_code::k8888);
}

// NORMAL

/**
 * @brief Checks if the vertex format uses  8-bit normals.
 *
 * This function determines whether the provided vertex format is configured
 * to use signed 8-bit fixed-point numbers for normal components.
 *
 * @param vertex_format The vertex format to check for 8-bit normal information.
 * @return True if the vertex format uses 8-bit normals.
 */
inline bool Is8Normal(u32 vertex_format) {
  return (vertexde::GetNormalFormat(vertex_format) == vertexde::fmt_code::k8);
}
/**
 * @brief Checks if the vertex format uses 16-bit normal components.
 *
 * This function determines whether the provided vertex format is configured
 * to use signed 16-bit fixed-point numbers for normal components.
 *
 * @param vertex_format The vertex format to check.
 * @return True if the vertex format uses signed 16-bit normal components.
 */
inline bool Is16Normal(u32 vertex_format) {
  return (vertexde::GetNormalFormat(vertex_format) == vertexde::fmt_code::k16);
}
/**
 * @brief Checks if the vertex format uses float normals.
 *
 * This function determines whether the provided vertex format is configured
 * to use float numbers for normal components.
 *
 * @param vertex_format The vertex format to check.
 * @return True if the vertex format uses float normals.
 */
inline bool Is32Normal(u32 vertex_format) {
  return (vertexde::GetNormalFormat(vertex_format) == vertexde::fmt_code::k32);
}
/**
 * @brief Checks if the vertex format uses fixed-point normals.
 *
 * @param vertex_format The vertex format to check.
 * @return True if the vertex format uses 8 or 16 bit fixed-point normal
 * components.
 */
inline bool IsFixedNormal(u32 vertex_format) { return (Is8Normal(vertex_format) || Is16Normal(vertex_format)); }

// POSITION
/**
 * @brief Checks if the vertex format uses 8-bit positions.
 *
 * This function determines whether the provided vertex format is configured
 * to use signed 8-bit fixed-point numbers for position components.
 *
 * @param vertex_format The vertex format to check.
 * @return True if the vertex format uses 8-bit positions.
 */
inline bool Is8Position(u32 vertex_format) {
  return (vertexde::GetPositionFormat(vertex_format) == vertexde::fmt_code::k8);
}
/**
 * @brief Checks if the vertex format uses 16-bit positions.
 *
 * This function determines whether the provided vertex format is configured
 * to use signed 16-bit fixed-point numbers for position components.
 *
 * @param vertex_format The vertex format to check.
 * @return True if the vertex format uses 16-bit positions.
 */
inline bool Is16Position(u32 vertex_format) {
  return (vertexde::GetPositionFormat(vertex_format) == vertexde::fmt_code::k16);
}
/**
 * @brief Checks if the vertex format uses float positions.
 *
 * This function determines whether the provided vertex format is configured
 * to use float numbers for position components.
 *
 * @param vertex_format The vertex format to check.
 * @return True if the vertex format uses float positions.
 */
inline bool Is32Position(u32 vertex_format) {
  return (vertexde::GetPositionFormat(vertex_format) == vertexde::fmt_code::k32);
}
/**
 * @brief Checks if the vertex format uses fixed-point positions.
 *
 * This function checks if the vertex format is configured
 * to use signed 8 or 16 bit fixed-point position components.
 *
 * @param vertex_format The vertex format to check.
 * @return True if the vertex format uses 8 or 16 bit fixed-point positions.
 */
inline bool IsFixedPosition(u32 vertex_format) { return (Is8Position(vertex_format) || Is16Position(vertex_format)); }
/**
 * @brief Generates a textual description of the vertex format.
 *
 * This function constructs a detailed description of the given vertex format
 * that includes information about used drawing modes and vertex attributes.
 *
 * @param vertex_format The vertex format to describe.
 * @return A string containing the description of the vertex format.
 */
std::string GetDescription(u32 vertex_format);
/**
 * @brief Structure representing the layout of a vertex.
 *
 * This structure holds the offsets for various components of a vertex
 * and its total byte size. It describes a **single** morph target.
 *
 *
 * @see nnl::vertexde::GetLayout
 * @see nnl::vertexde::fmt::kMorphNum
 */
struct VertexLayout {
  u32 offset_weights = 0;
  u32 offset_uv = 0;
  u32 offset_color = 0;
  u32 offset_normal = 0;
  u32 offset_position = 0;
  u32 vertex_size = 0;
};
/**
 * @brief Retrieves the layout of a vertex based on its format.
 *
 * This function calculates the offsets for various components of a vertex
 * and its total byte size. It returns a description of a **single** morph
 * target.
 *
 * @param vertex_format The format of the vertex to retrieve the layout for.
 * @return A VertexLayout structure containing the offsets and total size.
 *
 * @note If a vertex attribute is missing, its corresponding offset stores
 * the position where it would be located. To check for the presence of various
 * attributes, the family of Has* functions can be used.
 */
VertexLayout GetLayout(u32 vertex_format);
/** @} */

}  // namespace vertexde
/** @} */
}  // namespace nnl
