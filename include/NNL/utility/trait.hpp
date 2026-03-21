/**
 * @file trait.hpp
 * @brief Provides additional type trait utilities.
 *
 */
#pragma once

#include <iterator>
#include <type_traits>
namespace nnl {
/**
 * @copydoc trait.hpp
 *
 */
namespace utl::trait {
/**
 * \defgroup Traits Traits
 * @ingroup Utils
 * @copydoc trait.hpp
 * @{
 */
template <typename... Ts>
struct make_void {
  typedef void type;
};
/**
 * @brief A type trait similar to std::void_t that allows for
 * discarding overloads using SFINAE.
 *
 */
template <typename... Ts>
using void_t = typename make_void<Ts...>::type;

template <typename T, typename = void>
struct has_data_member : std::false_type {};

template <typename T>
struct has_data_member<T, utl::trait::void_t<decltype(std::declval<T>().data())>> : std::true_type {};
/**
 * @brief Type trait to detect if a type T has a `data()` member function.
 *
 * @tparam T The type to inspect.
 */
template <typename T>
constexpr bool has_data_member_v = has_data_member<T>::value;

template <typename T, typename = void>
struct has_size_member : std::false_type {};

template <typename T>
struct has_size_member<T, utl::trait::void_t<decltype(std::declval<T>().size())>> : std::true_type {};
/**
 * @brief Type trait to detect if a type T has a `size()` member function.
 *
 * @tparam T The type to inspect.
 */
template <typename T>
constexpr bool has_size_member_v = has_size_member<T>::value;

template <typename T, typename = void>
struct has_resize_member : std::false_type {};

template <typename T>
struct has_resize_member<T, utl::trait::void_t<decltype(std::declval<T>().resize(0))>> : std::true_type {};
/**
 * @brief Type trait to detect if a type T has a `resize()` member function.
 *
 * @tparam T The type to inspect.
 */
template <typename T>
constexpr bool has_resize_member_v = has_resize_member<T>::value;

template <typename, typename = void>
struct has_value_type : std::false_type {};

template <typename T>
struct has_value_type<T, utl::trait::void_t<typename T::value_type>> : std::true_type {};
/**
 * @brief Type trait to detect if a type T defines a nested `value_type`.
 *
 * @tparam T The type to inspect.
 */
template <typename T>
constexpr bool has_value_type_v = has_value_type<T>::value;

template <typename T, typename = void>
struct is_iterable : std::false_type {};

template <typename T>
struct is_iterable<T, std::void_t<decltype(std::begin(std::declval<T&>())), decltype(std::end(std::declval<T&>()))>>
    : std::true_type {};
/**
 * @brief Type trait to check if a type T is iterable.
 *
 *
 * @tparam T The type to inspect.
 */
template <typename T>
constexpr bool is_iterable_v = is_iterable<T>::value;
/**
 * @brief Type trait to check if a type T represents a contiguous range.
 *
 *
 * @tparam T The type to inspect.
 */
template <typename T>
constexpr bool is_contiguous_v = is_iterable_v<T> && has_data_member_v<T> && has_size_member_v<T>;
/**
 * @brief Type trait to check if a type T represents a non-contiguous range.
 *
 *
 * @tparam T The type to inspect.
 */
template <typename T>
constexpr bool is_non_contiguous_v = is_iterable_v<T> && !has_data_member_v<T>;

/** @} */
}  // namespace utl::trait
}  // namespace nnl
