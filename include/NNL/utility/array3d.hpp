/**
 * @file array3d.hpp
 * @brief Defines a 3d array class with a linear storage.
 */
#pragma once
#include <vector>

#include "NNL/common/contract.hpp"
#include "NNL/common/exception.hpp"
namespace nnl {

namespace utl {
/**
 * \addtogroup UtilityContainers
 * @{
 */
/**
 * @brief A 3-dimensional array template class.
 *
 * Provides access to elements via 3D coordinates, utilizing a linear array
 * under the hood for efficient storage.
 *
 * @tparam T The type of elements stored in the array.
 *
 * @see nnl::collision::Collision
 * @see nnl::shadow_collision::Collision
 */
template <typename T>
class Array3D {
 public:
  Array3D() noexcept {}
  /**
   * @brief Constructs a 3D array with specified dimensions.
   *
   * @param z The size in the z dimension.
   * @param y The size in the y dimension.
   * @param x The size in the x dimension.
   */
  Array3D(std::size_t z, std::size_t y, std::size_t x) : sz_(z), sy_(y), sx_(x) { data_.resize(sz_ * sy_ * sx_, T()); }
  /**
   * @brief Resizes the 3D array to specified dimensions.
   *
   * @param z The new size in the z dimension.
   * @param y The new size in the y dimension.
   * @param x The new size in the x dimension.
   * @note Existing data_ are not properly relocated by this method.
   */
  void Resize(std::size_t z, std::size_t y, std::size_t x) {
    sz_ = z;
    sy_ = y;
    sx_ = x;
    data_.resize(sz_ * sy_ * sx_, T{});
  }
  /**
   * @brief Returns the size of the array in the z dimension.
   *
   * @return The size in the z dimension.
   */
  std::size_t SizeZ() const noexcept { return sz_; }
  /**
   * @brief Returns the size of the array in the y dimension.
   *
   * @return The size in the y dimension.
   */
  std::size_t SizeY() const noexcept { return sy_; }
  /**
   * @brief Returns the size of the array in the x dimension.
   *
   * @return The size in the x dimension.
   */
  std::size_t SizeX() const noexcept { return sx_; }
  /**
   * @brief Accesses an element at specified coordinates with bounds checking.
   *
   * @param z The z coordinate of the element.
   * @param y The y coordinate of the element.
   * @param x The x coordinate of the element.
   * @return Reference to the specified element.
   */
  T& At(std::size_t z, std::size_t y, std::size_t x) {
    std::size_t index = GetIndex(z, y, x);
    if (index >= data_.size()) NNL_THROW(RangeError(NNL_SRCTAG("index out of range: " + std::to_string(index))));
    return data_.at(index);
  }
  /**
   * @brief Accesses an element at specified coordinates with bounds checking.
   *
   * @param z The z coordinate of the element.
   * @param y The y coordinate of the element.
   * @param x The x coordinate of the element.
   * @return Reference to the specified element.
   */
  const T& At(std::size_t z, std::size_t y, std::size_t x) const {
    std::size_t index = GetIndex(z, y, x);
    if (index >= data_.size()) NNL_THROW(RangeError(NNL_SRCTAG("index out of range: " + std::to_string(index))));
    return data_.at(index);
  }
  /**
   * @brief Accesses an element at specified coordinates (**no** bounds checking).
   *
   * @param z The z coordinate of the element.
   * @param y The y coordinate of the element.
   * @param x The x coordinate of the element.
   * @return Reference to the specified element.
   */
  T& operator()(std::size_t z, std::size_t y, std::size_t x) noexcept {
    NNL_EXPECTS_DBG(GetIndex(z, y, x) < data_.size());
    return data_[GetIndex(z, y, x)];
  }
  /**
   * @brief Accesses an element at specified coordinates (**no** bounds checking).
   *
   * @param z The z coordinate of the element.
   * @param y The y coordinate of the element.
   * @param x The x coordinate of the element.
   * @return Reference to the specified element.
   */
  const T& operator()(std::size_t z, std::size_t y, std::size_t x) const noexcept {
    NNL_EXPECTS_DBG(GetIndex(z, y, x) < data_.size());
    return data_[GetIndex(z, y, x)];
  }

 private:
  inline std::size_t GetIndex(std::size_t z, std::size_t y, std::size_t x) const noexcept {
    return x + y * sx_ + sx_ * sy_ * z;
  }

  std::vector<T> data_;
  std::size_t sz_;
  std::size_t sy_;
  std::size_t sx_;
};
/** @} */
}  // namespace utl
}  // namespace nnl
