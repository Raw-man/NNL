#include "NNL/simple_asset/svalue.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <utility>
namespace nnl {
SValue::SValue(std::initializer_list<SValue> init) {
  // Rule: If every element in the list is an Array of exactly size 2,
  // AND the first element of each is a String, it's an Object.
  bool is_object = !init.begin()->IsNull() && std::all_of(init.begin(), init.end(), [](const SValue& v) {
    return v.IsArray() && v.Size() == 2 && v[0].IsString();
  });

  if (is_object) {
    Object obj;
    for (const auto& element : init) {
      obj.emplace(element[0].Get<std::string>(), element[1]);
    }
    data_ = std::move(obj);
  } else {
    data_ = Array(init.begin(), init.end());
  }
}

bool SValue::AsBool() const noexcept {
  switch (GetType()) {
    case kNull:
      return false;
    case kBool:
      return std::get<bool>(data_);
    case kInt:
      return static_cast<bool>(std::get<i64>(data_));
    case kDouble:
      return static_cast<bool>(std::get<double>(data_));
    case kString:
      return !std::get<std::string>(data_).empty();
    case kObject:
      return !std::get<Object>(data_).empty();
    case kArray:
      return !std::get<Array>(data_).empty();
  }
  assert(0);
  return false;
}

double SValue::AsDouble() const {
  switch (GetType()) {
    case kNull:
      return 0.0;
    case kBool:
      return static_cast<double>(std::get<bool>(data_));
    case kInt:
      return static_cast<double>(std::get<i64>(data_));
    case kDouble:
      return std::get<double>(data_);
    case kString: {
      char* endptr;
      errno = 0;
      auto& str = std::get<std::string>(data_);
      double res = std::strtod(str.data(), &endptr);
      if (errno == 0 && str.data() != endptr) return res;
    }
      [[fallthrough]];
    default:
      NNL_THROW(RuntimeError(NNL_ERMSG("not a number")));
  }
}

i64 SValue::AsInt() const {
  switch (GetType()) {
    case kNull:
      return 0;
    case kBool:
      return static_cast<i64>(std::get<bool>(data_));
    case kInt:
      return std::get<i64>(data_);
    case kDouble:
      return static_cast<i64>(std::get<double>(data_));
    case kString: {
      char* endptr;
      errno = 0;
      auto& str = std::get<std::string>(data_);
      long res = std::strtol(str.data(), &endptr, 0);
      if (errno == 0 && str.data() != endptr) return static_cast<i64>(res);
    }
      [[fallthrough]];
    default:
      NNL_THROW(RuntimeError(NNL_ERMSG("not a number")));
  }
}

std::string SValue::AsString() const {
  switch (GetType()) {
    case kNull:
      return "null";
    case kBool:
      return std::get<bool>(data_) ? "true" : "false";
    case kInt:
      return std::to_string(std::get<i64>(data_));
    case kDouble:
      return std::to_string(std::get<double>(data_));
    case kString:
      return std::get<std::string>(data_);
    case kArray:
      return "[...]";
    case kObject:
      return "{...}";
  }

  assert(0);

  return "";
}

std::string SValue::DumpJson() const {
  std::stringstream s;
  DumpJsonImp(s);
  return s.str();
}

SValue& SValue::operator[](std::string_view key) {
  if (this->IsNull()) {
    *this = SValue::Object{};
  }
  NNL_EXPECTS(this->IsObject());

  auto& object_ = std::get<Object>(data_);

  auto it = object_.find(key);

  if (it != object_.end()) {
    return it->second;
  } else {
    auto [new_it, inserted] = object_.emplace(std::string(key), SValue{});

    return new_it->second;
  }
}

SValue& SValue::operator[](const char* key) { return this->operator[](std::string_view(key)); }

const SValue& SValue::operator[](std::string_view key) const { return this->At(key); }

const SValue& SValue::operator[](const char* key) const { return this->operator[](std::string_view(key)); }

const SValue& SValue::At(std::size_t ind) const {
  NNL_EXPECTS(this->IsArray());

  const auto& array_ = std::get<Array>(data_);

  if (ind >= array_.size()) {
    NNL_THROW(RangeError(NNL_ERMSG("index is out of range: " + std::to_string(ind))));
  }

  return array_[ind];
}

SValue& SValue::At(std::size_t ind) { return const_cast<SValue&>(std::as_const(*this).At(ind)); }

const SValue& SValue::At(std::string_view key) const {
  NNL_EXPECTS(this->IsObject());

  const auto& object_ = std::get<Object>(data_);
  Object::const_iterator it_key = object_.find(key);
  if (it_key == object_.end()) {
    NNL_THROW(RangeError(NNL_ERMSG("key is not in the object: " + std::string{key})));
  }
  return it_key->second;
}

SValue& SValue::At(std::string_view key) { return const_cast<SValue&>(std::as_const(*this).At(key)); }

const SValue* SValue::Find(std::string_view key) const noexcept {
  if (!IsObject()) return nullptr;

  const auto& object_ = std::get<Object>(data_);

  auto it = object_.find(key);
  return (it != object_.end()) ? &(it->second) : nullptr;
}

SValue* SValue::Find(std::string_view key) noexcept { return const_cast<SValue*>(std::as_const(*this).Find(key)); }

bool SValue::Has(std::string_view key) const noexcept { return Find(key) != nullptr; }

const SValue::Object& SValue::Items() const {
  NNL_EXPECTS(this->IsObject());
  return std::get<Object>(data_);
}

SValue::Object& SValue::Items() {
  if (this->IsNull()) *this = SValue::Object{};
  NNL_EXPECTS(this->IsObject());
  return std::get<Object>(data_);
}

const SValue::Array& SValue::Values() const {
  NNL_EXPECTS(this->IsArray());
  return std::get<Array>(data_);
}

SValue::Array& SValue::Values() {
  if (this->IsNull()) *this = SValue::Array{};
  NNL_EXPECTS(this->IsArray());
  return std::get<Array>(data_);
}

std::size_t SValue::Size() const noexcept {
  switch (GetType()) {
    case kNull:
      return 0;
    case kString:
      return std::get<std::string>(data_).size();
    case kArray:
      return std::get<Array>(data_).size();
    case kObject:
      return std::get<Object>(data_).size();
    default:
      return 1;
  }
}

SValue& SValue::Push(SValue val) {
  if (this->IsNull()) {
    *this = SValue::Array{};
  }
  NNL_EXPECTS(this->IsArray());
  auto& array_ = std::get<Array>(data_);
  array_.push_back(std::move(val));
  return array_.back();
}

SValue SValue::Pop() {
  NNL_EXPECTS(this->IsArray());
  NNL_EXPECTS(!this->IsEmpty());
  auto& array_ = std::get<Array>(data_);
  SValue back = std::move(array_.back());
  array_.pop_back();
  return back;
}

SValue& SValue::Insert(std::size_t pos, SValue val) {
  if (this->IsNull()) {
    *this = SValue::Array{};
  }

  NNL_EXPECTS(this->IsArray());

  auto& array_ = std::get<Array>(data_);

  NNL_EXPECTS(pos <= array_.size());

  auto it = array_.insert(array_.begin() + pos, std::move(val));

  return (*it);
}

std::pair<SValue&, bool> SValue::Insert(std::string_view key, SValue val) {
  if (this->IsNull()) {
    *this = SValue::Object{};
  }

  NNL_EXPECTS(this->IsObject());

  auto& object_ = std::get<Object>(data_);

  auto it = object_.find(key);

  if (it != object_.end()) {
    return {(it->second), false};
  }

  auto [new_it, inserted] = object_.emplace(std::string(key), std::move(val));

  return {(new_it->second), true};
}

bool SValue::Erase(std::size_t pos) {
  if (!IsArray()) return false;

  auto& array_ = std::get<Array>(data_);
  if (pos >= array_.size()) return false;

  array_.erase(array_.begin() + pos);
  return true;
}

bool SValue::Erase(std::string_view key) {
  if (!IsObject()) return false;

  auto& object_ = std::get<Object>(data_);

  auto it = object_.find(key);

  if (it != object_.end()) {
    object_.erase(it);
    return true;
  }

  return false;
}

void SValue::Clear() {
  switch (GetType()) {
    case kNull:
      data_ = nullptr;
      break;
    case kBool:
      data_ = false;
      break;
    case kInt:
      data_ = 0;
      break;
    case kDouble:
      data_ = 0.0;
      break;
    case kString:
      std::get<std::string>(data_).clear();
      break;
    case kArray:
      std::get<Array>(data_).clear();
      break;
    case kObject:
      std::get<Object>(data_).clear();
      break;
  }
}

bool SValue::operator==(const SValue& rhs) const noexcept {
  if (this->GetType() == rhs.GetType()) return data_ == rhs.data_;

  if ((this->IsNumber() && rhs.IsNumber())) {
    return this->AsDouble() == rhs.AsDouble();
  }

  return false;
}

bool SValue::operator!=(const SValue& rhs) const noexcept { return !(this->operator==(rhs)); }

void SValue::DumpJsonImp(std::ostream& scout, int indent_level) const {
  std::string indent(indent_level, ' ');

  switch (GetType()) {
    case kNull:
      scout << "null";
      break;
    case kBool:
      scout << (std::get<bool>(data_) ? "true" : "false");
      break;
    case kInt:
      scout << std::get<i64>(data_);
      break;
    case kDouble:
      scout << std::get<double>(data_);
      break;
    case kString:
      scout << std::quoted(std::get<std::string>(data_));
      break;
    case kArray: {
      const auto& arr = Values();
      if (arr.empty()) {
        scout << "[]";
      } else {
        scout << "[\n";
        for (size_t i = 0; i < arr.size(); ++i) {
          scout << indent << "  ";
          arr[i].DumpJsonImp(scout, indent_level + 2);
          if (i < arr.size() - 1) scout << ",";
          scout << "\n";
        }
        scout << indent << "]";
      }
      break;
    }
    case kObject: {
      const auto& obj = Items();
      if (obj.empty()) {
        scout << "{}";
      } else {
        scout << "{\n";
        size_t i = 0;
        for (const auto& [key, val] : obj) {
          scout << indent << "  \"" << key << "\": ";
          val.DumpJsonImp(scout, indent_level + 2);
          if (++i < obj.size()) scout << ",";
          scout << "\n";
        }
        scout << indent << "}";
      }
      break;
    }
  }
}

}  // namespace nnl
