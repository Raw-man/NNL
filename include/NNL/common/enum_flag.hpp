
/**
 * @file enum_flag.hpp
 * @brief Contains macros for defining flag enums.
 */

#include <type_traits>  // IWYU pragma: export

/**
 * \defgroup FlagEnum Flag Enums
 * @ingroup Common
 * @copydoc enum_flag.hpp
 *
 * @{
 */

/**
 * @def NNL_FLAG_ENUM
 * @brief An attribute for defining flag enums
 *
 *  Indicates that an enumeration represents a set of bitwise flags, where any
 *  combination of values is a valid case.
 *
 */
#if defined(__has_cpp_attribute) && __has_cpp_attribute(clang::flag_enum)
#define NNL_FLAG_ENUM [[clang::flag_enum]]
#else
#define NNL_FLAG_ENUM
#endif

/**
 * @def NNL_DEFINE_ENUM_OPERATORS
 * @brief Macro to define bitwise operators for enum types
 * @param ENUMTYPE The enum type to generate operators for
 */
#define NNL_DEFINE_ENUM_OPERATORS(ENUMTYPE)                                          \
  inline ENUMTYPE operator|(ENUMTYPE a, ENUMTYPE b) {                                \
    return static_cast<ENUMTYPE>(static_cast<std::underlying_type_t<ENUMTYPE>>(a) |  \
                                 static_cast<std::underlying_type_t<ENUMTYPE>>(b));  \
  }                                                                                  \
  inline ENUMTYPE& operator|=(ENUMTYPE& a, ENUMTYPE b) { return a = a | b; }         \
  inline ENUMTYPE operator&(ENUMTYPE a, ENUMTYPE b) {                                \
    return static_cast<ENUMTYPE>(static_cast<std::underlying_type_t<ENUMTYPE>>(a) &  \
                                 static_cast<std::underlying_type_t<ENUMTYPE>>(b));  \
  }                                                                                  \
  inline ENUMTYPE operator&=(ENUMTYPE& a, ENUMTYPE b) { return a = a & b; }          \
  inline ENUMTYPE operator~(ENUMTYPE a) {                                            \
    return static_cast<ENUMTYPE>(~static_cast<std::underlying_type_t<ENUMTYPE>>(a)); \
  }                                                                                  \
  inline ENUMTYPE operator^(ENUMTYPE a, ENUMTYPE b) {                                \
    return static_cast<ENUMTYPE>(static_cast<std::underlying_type_t<ENUMTYPE>>(a) ^  \
                                 static_cast<std::underlying_type_t<ENUMTYPE>>(b));  \
  }                                                                                  \
  inline ENUMTYPE operator^=(ENUMTYPE& a, ENUMTYPE b) { return a = a ^ b; }
/** @} */
