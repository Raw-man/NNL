/**
 * @file static_set.hpp
 *
 * @brief Defines a set that is sorted and has a fixed capacity.
 *
 */

#pragma once

#include <algorithm>
#include <array>

#include "NNL/common/contract.hpp"
#include "NNL/common/exception.hpp"

namespace nnl {
namespace utl {
/**
 * \addtogroup UtilityContainers
 * @{
 */
/**
 * @brief Fixed-size sorted set implementation
 * @tparam T Arithmetic type for elements
 * @tparam max_size Maximum number of elements
 */
template <typename T, std::size_t max_size>
class StaticSet {
  static_assert(std::is_arithmetic_v<T>);

 public:
  using container_type = typename std::array<T, max_size>;
  using iterator = typename container_type::const_iterator;
  using const_iterator = typename container_type::const_iterator;
  using value_type = T;

  StaticSet() noexcept {}

  StaticSet(std::initializer_list<T> list) {
    for (const auto& val : list) Insert(val);
  }

  bool Insert(T value) {
    std::size_t i = 0;
    for (; i < size_; i++) {
      if (data_[i] == value) return false;

      if (data_[i] > value) {
        for (std::size_t j = size_; j > i && size_ + 1 <= max_size; j--) {
          assert(j != 0);
          data_[j] = data_[j - 1];
        }
        break;
      }
    }

    if (size_ + 1 > max_size) {
      NNL_THROW(RangeError(NNL_SRCTAG("not enough space")));
    }

    data_[i] = value;

    size_++;

    return true;
  }

  bool Contains(T value) const noexcept {
    return std::find(data_.begin(), data_.begin() + size_, value) != data_.begin() + size_;
  }

  [[nodiscard]] StaticSet<T, max_size> Join(const StaticSet<T, max_size>& other) const {
    StaticSet<T, max_size> result;

    std::size_t i = 0, j = 0, k = 0;

    // Merge the two sorted ranges
    while (i < size_ && j < other.size_ && k < max_size) {
      if (data_[i] == other.data_[j]) {
        result.data_[k] = data_[i];
        i++;
        j++;
      } else if (data_[i] < other.data_[j]) {
        result.data_[k] = data_[i];
        i++;
      } else {
        result.data_[k] = other.data_[j];
        j++;
      }
      k++;
    }
    // Add remaining elements
    while (i < size_ && k < max_size) {
      result.data_[k] = data_[i];
      i++;
      k++;
    }

    while (j < other.size_ && k < max_size) {
      result.data_[k] = other.data_[j];
      j++;
      k++;
    }

    result.size_ = k;

    if (i != size_ || j != other.size_) {
      NNL_THROW(RangeError(NNL_SRCTAG("not enough space")));
    }

    assert(result.size_ >= size_ && result.size_ >= other.size_);
    return result;
  }

  [[nodiscard]] StaticSet<T, max_size> Intersect(const StaticSet<T, max_size>& other) const {
    StaticSet<T, max_size> new_set;
    auto new_end = std::set_intersection(data_.begin(), data_.begin() + size_, other.data_.begin(),
                                         other.data_.begin() + other.size_, new_set.data_.begin());

    new_set.size_ = static_cast<std::size_t>(std::distance(new_set.data_.begin(), new_end));

    return new_set;
  }

  [[nodiscard]] StaticSet<T, max_size> Difference(const StaticSet<T, max_size>& other) const {
    StaticSet<T, max_size> new_set;
    auto new_end = std::set_difference(data_.begin(), data_.begin() + size_, other.data_.begin(),
                                       other.data_.begin() + other.size_, new_set.data_.begin());

    new_set.size_ = static_cast<std::size_t>(std::distance(new_set.data_.begin(), new_end));

    return new_set;
  }

  bool IsSubset(const StaticSet<T, max_size>& other) const {
    if (other.Size() > Size()) return false;

    return std::includes(data_.begin(), data_.begin() + size_, other.data_.begin(), other.data_.begin() + other.size_);
  }

  std::size_t Size() const noexcept { return size_; }

  bool IsEmpty() const noexcept { return size_ == 0; }

  void Clear() noexcept { size_ = 0; }

  const T& operator[](std::size_t index) const noexcept {
    NNL_EXPECTS_DBG(index < this->Size());
    return data_[index];
  }

  bool operator==(const StaticSet& rhs) const noexcept {
    if (rhs.Size() != Size()) return false;

    for (std::size_t i = 0; i < rhs.Size(); i++)
      if (data_[i] != rhs.data_[i]) return false;

    return true;
  }

  const_iterator begin() const noexcept { return data_.cbegin(); }

  const_iterator end() const noexcept { return data_.cbegin() + size_; }

  const_iterator cbegin() const noexcept { return data_.cbegin(); }

  const_iterator cend() const noexcept { return data_.cbegin() + size_; }

  const value_type& front() const noexcept {
    NNL_EXPECTS_DBG(!this->IsEmpty());
    return *data_.cbegin();
  }

  const value_type& back() const noexcept {
    NNL_EXPECTS_DBG(!this->IsEmpty());
    return *data_.rcbegin();
  }

 private:
  std::size_t size_ = 0;
  container_type data_;
};
/** @} */
}  // namespace utl
}  // namespace nnl
