/**
 * @file svalue.hpp
 * @brief Provides a class for representing "extras" fields for custom metadata.
 */

#pragma once

#include <ostream>
#include <map>
#include <string>
#include <variant>
#include <vector>

#include "NNL/common/fixed_type.hpp"
#include "NNL/common/contract.hpp"   // IWYU pragma: export
#include "NNL/common/exception.hpp"  // IWYU pragma: export

namespace nnl {

/**
 * \defgroup SimpleValue Simple Value
 * @ingroup SimpleAssets
 * @copydoc svalue.hpp
 * @{
 */

/**
 * @brief A class representing JSON-like values (null, bool, numbers,
 * strings, arrays, or objects)
 *
 * @code
 * nnl::SValue extras;
 * extras["id"] = 1001;
 * extras["material_1"]["color"] = "#FF0000";
 * extras["tags"].Push("unlit");
 * i64 id = extras["id"];
 * std::string color = extras["material_1"]["color"];
 * std::string tag = extras["tags"][0];
 * @endcode
 */
class SValue {
 public:
  using Array = std::vector<SValue>;
  using Object = std::map<std::string, SValue, std::less<>>;

  enum Type { kNull, kBool, kInt, kDouble, kString, kArray, kObject };

  // --- Construction (Converting) ---
  SValue() : data_(nullptr) {}

  SValue(std::nullptr_t) : data_(nullptr) {}

  SValue(bool b) : data_(b) {}

  template <typename T, std::enable_if_t<std::is_integral_v<T>>* = nullptr>
  SValue(T int_num) : data_(static_cast<i64>(int_num)) {}

  template <typename T, std::enable_if_t<std::is_floating_point_v<T>>* = nullptr>
  SValue(T real_num) : data_(static_cast<double>(real_num)) {}

  template <typename T, std::enable_if_t<std::is_constructible_v<std::string, T>>* = nullptr>
  SValue(T&& str) : data_(std::string(std::forward<T>(str))) {}

  SValue(const Array& a) : data_(a) {}
  SValue(Array&& a) noexcept : data_(std::move(a)) {}

  SValue(const Object& o) : data_{o} {}
  SValue(Object&& o) noexcept : data_(std::move(o)) {}

  SValue(std::initializer_list<SValue> init);

  ~SValue() = default;

  SValue(const SValue&) = default;

  SValue(SValue&&) noexcept = default;

  SValue& operator=(const SValue&) = default;

  SValue& operator=(SValue&&) noexcept = default;

  // --- Implicit Conversions ---
  operator bool() const { return this->AsBool(); }

  template <typename T, std::enable_if_t<std::is_integral_v<T>>* = nullptr>
  operator T() const {
    return static_cast<T>(this->AsInt());
  }

  template <typename T, std::enable_if_t<std::is_floating_point_v<T>>* = nullptr>
  operator T() const {
    return static_cast<T>(this->AsDouble());
  }

  operator std::string() const { return this->AsString(); }

  // --- Inspection ---

  Type GetType() const noexcept { return static_cast<Type>(data_.index()); }

  bool IsNull() const noexcept { return GetType() == kNull; };

  bool IsBool() const noexcept { return GetType() == kBool; }

  bool IsInt() const noexcept { return GetType() == kInt; }

  bool IsDouble() const noexcept { return GetType() == kDouble; }

  bool IsNumber() const noexcept { return IsInt() || IsDouble(); }

  bool IsString() const noexcept { return GetType() == kString; }

  bool IsArray() const noexcept { return GetType() == kArray; }

  bool IsObject() const noexcept { return GetType() == kObject; }

  bool IsEmpty() const noexcept { return Size() == 0; }

  bool Has(std::string_view key) const noexcept;

  std::size_t Size() const noexcept;

  const SValue* Find(std::string_view key) const noexcept;

  SValue* Find(std::string_view key) noexcept;

  // --- Data Access ---

  const Object& Items() const;

  Object& Items();

  const Array& Values() const;

  Array& Values();

  bool AsBool() const noexcept;

  double AsDouble() const;

  i64 AsInt() const;

  std::string AsString() const;

  std::string DumpJson() const;

  SValue& operator[](std::string_view key);

  SValue& operator[](const char* key);

  const SValue& operator[](std::string_view key) const;

  const SValue& operator[](const char* key) const;

  template <typename T, std::enable_if_t<std::is_integral_v<T>>* = nullptr>
  SValue& operator[](T ind) {
    if (this->IsNull()) {
      *this = SValue::Array{};
    }

    NNL_EXPECTS(this->IsArray());

    auto& array_ = std::get<Array>(data_);

    if (static_cast<std::size_t>(ind) >= array_.size()) {
      array_.resize(ind + 1);
    }

    return array_[ind];
  }

  template <typename T, std::enable_if_t<std::is_integral_v<T>>* = nullptr>
  const SValue& operator[](T ind) const {
    NNL_EXPECTS(this->IsArray());
    auto& array_ = std::get<Array>(data_);
    NNL_EXPECTS_DBG(static_cast<std::size_t>(ind) < array_.size());
    return array_[ind];
  }

  template <typename T>
  const T& Get() const {
    NNL_EXPECTS(std::holds_alternative<T>(data_));
    return std::get<T>(data_);
  }

  template <typename T>
  T& Get() {
    NNL_EXPECTS(std::holds_alternative<T>(data_));
    return std::get<T>(data_);
  }

  template <typename T>
  const T* GetIf() const noexcept {
    return std::get_if<T>(&data_);
  }

  template <typename T>
  T* GetIf() noexcept {
    return std::get_if<T>(&data_);
  }

  const SValue& At(std::size_t ind) const;

  SValue& At(std::size_t ind);

  const SValue& At(std::string_view key) const;

  SValue& At(std::string_view key);

  // --- Modification ---

  SValue& Push(SValue val);

  SValue Pop();

  SValue& Insert(std::size_t pos, SValue val);

  std::pair<SValue&, bool> Insert(std::string_view key, SValue val);

  bool Erase(std::size_t pos);

  bool Erase(std::string_view key);

  void Clear();

  bool operator==(const SValue& rhs) const noexcept;

  bool operator!=(const SValue& rhs) const noexcept;

  friend inline std::ostream& operator<<(std::ostream& os, const SValue& v) {
    os << v.AsString();
    return os;
  }

 protected:
  using value_type = std::variant<std::nullptr_t, bool, i64, double, std::string, Array, Object>;
  value_type data_;

  void DumpJsonImp(std::ostream& scout, int indent_level = 0) const;
};
/** @} */
}  // namespace nnl
