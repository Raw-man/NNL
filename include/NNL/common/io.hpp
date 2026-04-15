/**
 * @file io.hpp
 * @brief Provides classes for reading and writing binary data to and
 * from various sources.
 *
 *
 * @see nnl::Reader
 * @see nnl::Writer
 * @see nnl::BufferView
 * @see nnl::BufferRW
 * @see nnl::FileReader
 * @see nnl::FileRW
 * @see nnl::BufferSpan
 *
 */
#pragma once
#include <filesystem>
#include <fstream>
#include <type_traits>
#include <vector>

#include "NNL/common/exception.hpp"
#include "NNL/common/fixed_type.hpp"
#include "NNL/utility/filesys.hpp"
#include "NNL/utility/math.hpp"

namespace nnl {

/**
 * \defgroup InputOutput Input/Output
 * @ingroup Common
 * @copydoc io.hpp
 * @{
 */

/**
 * @brief A type alias for std::vector<u8> that denotes a raw, contiguous memory region that may be
 * interpreted in multiple ways.
 */
using Buffer = std::vector<u8>;

/**
 * @class nnl::RWBase
 * @brief Base interface for data reading/writing classes
 *
 * Provides basic functionality for data "streams".
 *
 * @see nnl::Reader
 * @see nnl::Writer
 */
class RWBase {
 public:
  /**
   * @brief Seeks to a specific offset in the data.
   * @param offset The offset to seek to.
   */
  virtual void Seek(std::size_t offset) = 0;
  /**
   * @brief Gets the current position in the data.
   * @return The current position as a size_t.
   */
  virtual std::size_t Tell() = 0;
  /**
   * @brief Gets the available length of the data.
   * @return The length of the data.
   */
  virtual std::size_t Len() = 0;

  virtual ~RWBase() = default;
};
/**
 * @class nnl::Reader
 * @brief Abstract class for reading data.
 *
 * Inherits from `RWBase` and provides methods to read _**trivially copyable**_
 * data from various sources.
 */
class Reader : virtual public RWBase {
 public:
  /**
   * @brief Reads a block of data into the specified destination.
   * @param dst Pointer to the destination buffer.
   * @param size Size of the data to read in bytes.
   */
  void ReadBuf(void* dst, std::size_t size) { ReadData(dst, size); }
  /**
   * @brief Reads a trivially copyable object
   * @tparam T Type to read (must be trivially copyable)
   * @return The read value
   */
  template <typename T, typename = std::enable_if_t<std::is_trivially_copyable_v<T>>>
  T ReadLE() {
    T value;
    ReadData(&value, sizeof(T));
    return value;
  }
  /**
   * @brief Reads multiple trivially copyable objects into a
   * vector
   *
   * @tparam T Type to read (must be trivially copyable)
   * @param num_elements Number of elements to read
   * @return Container filled with read elements
   *
   */
  template <typename T>
  std::vector<T> ReadArrayLE(std::size_t num_elements) {
    static_assert(std::is_trivially_copyable_v<T>);
    std::vector<T> result;

    result.resize(num_elements);

    ReadData(result.data(), num_elements * sizeof(T));

    return result;
  }

  virtual ~Reader() override = default;

 protected:
  /**
   * @brief Low-level data reading implementation
   * @param dst Destination buffer
   * @param size Number of bytes to read
   */
  virtual void ReadData(void* dst, std::size_t size) = 0;
};

/**
 * @class nnl::Writer
 * @brief Abstract class for writing data.
 *
 * Inherits from `RWBase` and provides methods to write _**trivially copyable**_
 * data to various destinations.
 */
class Writer : virtual public RWBase {
 public:
  /**
   * @class nnl::Writer::Offset
   * @brief Pointer-like object for storing stream positions
   *
   * This object stores specific offsets from the main stream position
   * to provide easier access to the referenced objects.
   *
   * @code
   * std::size_t struct_offset = 0; // Not yet known
   * auto off = f.Write(struct_offset);
   * // ...
   * *off = f.Tell(); // Change the value
   * @endcode
   *
   * @tparam T Type being referenced by the offset
   * @see nnl::Writer::Ref
   *
   */
  template <typename T>
  class Offset {
   public:
    auto Get() const { return Ref<T>(*this); }

    template <typename M, typename A = T, std::enable_if_t<std::is_class_v<A>, int> = 0>
    auto Get(M A::*member) const {
      return this->Get().Get(member);
    }

    /**
     * @brief Gets a reference to T

     * @return Ref pointing to the member
     * @see nnl::Writer::Ref
     *
     */
    auto operator*() const { return this->Get(); }

    /**
     * @brief Gets a reference to a member of T (->)
     * @tparam M Member type (deduced)
     * @param member Pointer to member
     * @return Ref pointing to the member
     *
     * @see nnl::Writer::Ref
     */
    template <typename M, typename A = T, std::enable_if_t<std::is_class_v<A>, int> = 0>
    auto operator->*(M A::*member) const {
      return this->Get(member);
    }

    /**
     * @brief Gets a reference to the element at the index relative to the
     * current offset
     *
     * Enables array-like access.
     *
     * @param index index of the element.
     * @return Ref to the value at offset.
     * @see nnl::Writer::Ref
     */
    auto operator[](std::size_t index) const { return *((*this) + index); }

    /**
     * @brief Conversion to integer (returns the offset_ value)
     * @tparam Int Integer type
     * @return The offset_ value as integer
     */
    template <typename Int>
    explicit operator Int() const noexcept {
      static_assert(std::is_integral_v<Int>);
      return static_cast<Int>(offset_);
    }

    bool operator==(const Offset& rhs) const = delete;

    bool operator!=(const Offset& rhs) const = delete;

    Offset& operator++() {
      offset_ += sizeof(T);
      return *this;
    }

    Offset operator++(int) {
      Offset prev = *this;
      ++(*this);
      return prev;
    }

    Offset& operator+=(std::size_t value) {
      offset_ += sizeof(T) * value;
      return *this;
    }

    Offset operator+(std::size_t value) const {
      Offset result = *this;
      result += value;  // Reuse compound assignment
      return result;
    }

    Offset& operator--() {
      offset_ -= sizeof(T);
      return *this;
    }

    Offset operator--(int) {
      Offset prev = *this;
      --(*this);
      return prev;
    }

    Offset& operator-=(std::size_t value) { return *this -= value; }

    Offset operator-(std::size_t value) const {
      Offset result = *this;
      result -= value;
      return result;
    }

    friend Offset operator+(std::size_t value, const Offset& offset) {
      return Offset{offset.writer_, offset.offset_ + value};
    }

    friend Offset operator-(std::size_t value, const Offset& offset) {
      return Offset{offset.writer_, offset.offset_ - value};
    }

   private:
    Offset(Writer& writer, std::size_t offset) : writer_(writer), offset_(offset) {}

    Writer& writer_;
    std::size_t offset_;
    friend class Writer;
  };  // class Offset

  /**
   * @class nnl::Writer::Ref
   * @brief Reference-like object for writing and reading data at memorized
   * stream positions
   *
   * Allows writing and reading (if a derived class implements Reader) at
   * specific positions by temporarily changing the main "stream" position.
   *
   * @tparam T Type being referenced
   * @see nnl::Writer::Offset
   */
  template <typename T>
  class Ref {
   public:
    template <typename A = T>
    std::enable_if_t<!std::is_array_v<A>, T> Get() const {
      auto prev_offset = off_.writer_.Tell();
      off_.writer_.Seek(off_.offset_);
      auto value = dynamic_cast<Reader&>(off_.writer_).ReadLE<T>();
      off_.writer_.Seek(prev_offset);
      return value;
    }

    void Set(const T& value) {
      auto prev_offset = off_.writer_.Tell();
      off_.writer_.Seek(off_.offset_);
      off_.writer_.WriteData(&value, sizeof(T));
      off_.writer_.Seek(prev_offset);
    }

    template <typename M, typename A = T, std::enable_if_t<std::is_class_v<A>, int> = 0>
    Ref<M> Get(M A::*member) const {
      std::size_t member_offset = reinterpret_cast<std::size_t>(&(((T*)nullptr)->*member));
      return Ref<M>({off_.writer_, off_.offset_ + member_offset});
    }

    template <typename A = T, typename M = std::remove_all_extents_t<A>,
              std::enable_if_t<std::is_array_v<A> && std::rank_v<A> == 1, int> = 0>
    Ref<M> Get(std::size_t index) const {
      return Ref<M>({off_.writer_, off_.offset_ + sizeof(M) * index});
    }

    template <typename A = T, typename M = std::remove_extent_t<A>,
              std::enable_if_t<std::is_array_v<A> && std::rank_v<A> != 1, int> = 0>
    Ref<M> Get(std::size_t index) const {
      return Ref<M>({off_.writer_, off_.offset_ + sizeof(M) * index});
    }

    /**
     * @brief Creates an Ref to a member of the referenced struct
     * @tparam M Member type (deduced)
     * @param member Pointer to member
     * @return Ref to the member
     */
    template <typename M, typename A = T, std::enable_if_t<std::is_class_v<A>, int> = 0>
    Ref<M> operator->*(M A::*member) const {
      return Get(member);
    }
    /**
     * @brief Creates an Ref to an element of the referenced C array
     * @tparam M Element type (deduced)
     * @param index Array index
     * @return Ref pointing to the array element
     */
    template <typename A = T, typename M = std::remove_all_extents_t<A>,
              std::enable_if_t<std::is_array_v<A> && std::rank_v<A> == 1, int> = 0>
    Ref<M> operator[](std::size_t index) const {
      return Get(index);
    }
    /**
     * @brief Creates an Ref to a subdimension of the referenced
     * multidimensional C array
     * @tparam M Array type (deduced)
     * @param index Array index
     * @return Ref pointing to the array
     */
    template <typename A = T, typename M = std::remove_extent_t<A>,
              std::enable_if_t<std::is_array_v<A> && std::rank_v<A> != 1, int> = 0>
    Ref<M> operator[](std::size_t index) const {
      return Get(index);
    }

    /**
     * @brief Reads the entire object at the offset position
     * @return The object
     */
    template <typename A = T, std::enable_if_t<!std::is_array_v<A>, int> = 0>
    operator A() const {
      static_assert(std::is_convertible_v<T, A>);

      return Get();
    }

    /**
     * @brief Assignment operator for writing the entire object
     * @param value Object to write
     */
    void operator=(const T& value) { Set(value); }

    bool operator==(const Ref& other) const { return this->Get() == other.Get(); }

    bool operator!=(const Ref& other) const { return !(*this == other); }

    bool operator==(const T& other) const { return this->Get() == other; }

    bool operator!=(const T& other) const { return !(*this == other); }

   private:
    Ref(Offset<T> offset) : off_(offset) {}

    Offset<T> off_;

    friend class Writer;
  };  // class Ref

  /**
   * @brief Creates an Offset pointing to the current position
   * @tparam T Type of the object to reference
   * @return Offset object at current position
   */
  template <typename T, typename = std::enable_if_t<std::is_trivially_copyable_v<T>>>
  Offset<T> MakeOffsetLE() {
    return Offset<T>(*this, Tell());
  }
  /**
   * @brief Writes raw binary data
   * @param src Source buffer
   * @param size Number of bytes to write
   * @return Offset pointing to the written data
   */
  Offset<u8> WriteBuf(const void* src, std::size_t size) {
    auto pos = Tell();
    WriteData(src, size);
    return Offset<u8>(*this, pos);
  }

  /**
   * @brief Writes a trivially copyable value
   * @tparam T Value type (must be trivially copyable)
   * @param value Value to write
   * @return Offset pointing to the written value
   */
  template <typename T, std::enable_if_t<std::is_trivially_copyable_v<T>, int> = 0>
  Offset<T> WriteLE(const T& value) {
    auto pos = Tell();

    WriteData(&value, sizeof(T));

    return Offset<T>(*this, pos);
  }

  /**
   * @brief Writes contents of a vector
   *
   * @param container Container to write
   * @return Offset pointing to the first written element
   *
   */
  template <typename T>
  Offset<T> WriteArrayLE(const std::vector<T>& container) {
    static_assert(std::is_trivially_copyable_v<T>);

    auto prev_position = Tell();

    WriteData(container.data(), container.size() * sizeof(T));

    return Offset<T>(*this, prev_position);
  }

  /**
   * @brief Aligns the current position to a specified multiple.
   * @param multiple Alignment boundary (e.g., 4 for 4-byte alignment)
   * @param value Byte value to use for padding
   */
  void AlignData(std::size_t multiple, unsigned char value = 0) {
    auto cur_pos = Tell();
    auto num_bytes = utl::math::RoundNum(cur_pos, multiple) - cur_pos;
    for (std::size_t i = 0; i < num_bytes; i++) WriteLE(value);
  }

  virtual ~Writer() override = default;

 protected:
  /**
   * @brief Low-level data writing implementation
   * @param src Source buffer
   * @param size Number of bytes to write
   */
  virtual void WriteData(const void* src, std::size_t size) = 0;
};

/**
 * @class nnl::FileReader
 * @brief Reader implementation for reading from a file
 */
class FileReader final : public Reader {
 public:
  /**
   * @brief Constructs a FileReader from the specified file.
   *
   * @param path The filesystem path to the file.
   * @throw nnl::IOError
   */
  FileReader(const std::filesystem::path& path) : path_(path) {
    file_stream_.open(path, std::ios::ate | std::ios::in | std::ios::binary);

    if (!file_stream_.is_open()) {
      NNL_THROW(nnl::IOError(NNL_SRCTAG(std::strerror(errno)), utl::filesys::u8string(path)));
    }

    size_ = file_stream_.tellg();
    file_stream_.seekg(0);
  }
  void Seek(std::size_t offset) override { file_stream_.seekg(offset, std::ios::beg); }

  std::size_t Tell() override { return file_stream_.tellg(); }

  std::size_t Len() override { return size_; }
  /**
   * @brief Clears any error flags on the file stream
   */
  void ClearState() { file_stream_.clear(); }
  /**
   * @brief Closes the file
   */
  void Close() {
    file_stream_.close();
    size_ = 0;
  }

 protected:
  void ReadData(void* dst, std::size_t size) override {
    file_stream_.read(static_cast<char*>(dst), size);
    if (!file_stream_) {
      NNL_THROW(nnl::IOError(NNL_SRCTAG("failed to read data"), utl::filesys::u8string(path_)));
    }
  }

 private:
  std::filesystem::path path_;
  std::ifstream file_stream_;
  std::size_t size_ = 0;
};

/**
 * @class nnl::FileRW
 * @brief Reader/Writer implementation for file io operations.
 */
class FileRW final : public Writer, public Reader {
 public:
  /**
   * @brief Constructs a FileRW from the specified file.
   *
   * @param path The filesystem path to the file.
   * @param truncate If the contents should be discarded
   * @note The file must exist if truncate is false
   * @throw nnl::IOError
   */
  FileRW(const std::filesystem::path& path, bool truncate) : path_(path) {
    auto open_mode = std::ios::binary | std::ios::out | std::ios::in;

    if (truncate) {
      open_mode |= std::ios::trunc;
    }

    file_stream_.open(path, open_mode);

    if (!file_stream_.is_open()) {
      NNL_THROW(nnl::IOError(NNL_SRCTAG(std::strerror(errno)), utl::filesys::u8string(path)));
    }
  }
  void Seek(std::size_t offset) override { file_stream_.seekp(offset, std::ios::beg); }

  std::size_t Tell() override { return file_stream_.tellp(); }

  std::size_t Len() override {
    auto cur_pos = file_stream_.tellp();
    file_stream_.seekp(0, std::ios::end);
    std::size_t size = file_stream_.tellp();
    file_stream_.seekp(cur_pos);
    return size;
  }
  /**
   * @brief Clears any error flags on the file stream
   */
  void ClearState() { file_stream_.clear(); }
  /**
   * @brief Closes the file
   */
  void Close() { file_stream_.close(); }

 protected:
  void WriteData(const void* src, std::size_t size) override {
    file_stream_.write(static_cast<const char*>(src), size);
    if (!file_stream_) {
      NNL_THROW(nnl::IOError(NNL_SRCTAG("failed to write data"), utl::filesys::u8string(path_)));
    }
  }

  void ReadData(void* dst, std::size_t size) override {
    file_stream_.read(static_cast<char*>(dst), size);
    if (!file_stream_) {
      NNL_THROW(nnl::IOError(NNL_SRCTAG("failed to read data"), utl::filesys::u8string(path_)));
    }
  }

 private:
  std::filesystem::path path_;
  std::fstream file_stream_;
};
/**
 * @class nnl::BufferView
 * @brief Reader implementation for read-only memory buffers
 *
 * Provides a view into _existing_ memory without taking ownership.
 * Suitable for reading from buffers or _memory-mapped_ files.
 * Offers an interface similar to std::vector.
 *
 * @code
 * std::vector<unsigned char> buffer(64,0);
 * nnl::BufferView view{buffer};
 * nnl::BufferView c_view{buffer.data(), buffer.size()*sizeof(unsigned char)};
 * // Implicitly constructed
 * bool is_model = nnl::model::IsOfType(buffer);
 * @endcode
 */
class BufferView final : public Reader {
 public:
  using value_type = u8;
  using iterator = const value_type*;
  using const_iterator = const value_type*;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  BufferView() noexcept {}

  /**
   * @brief Constructs a BufferView from a pointer/size pair.
   * @param buffer Pointer to the buffer.
   * @param size The _byte size_ of the buffer.
   */
  BufferView(const void* buffer, std::size_t size) noexcept
      : buffer_ptr_(reinterpret_cast<const u8*>(buffer)), buffer_size_(size) {
    NNL_EXPECTS_DBG(buffer != nullptr || size == 0);
  }

  /**
   * @brief Constructs a BufferView from a generic contiguous container.
   *
   * @param buffer Reference to the container to use as the source.
   */
  template <typename TContainer,
            typename = std::enable_if_t<std::is_trivially_copyable_v<typename TContainer::value_type>>>
  BufferView(const TContainer& buffer) noexcept
      : buffer_ptr_(reinterpret_cast<const u8*>(buffer.data())),
        buffer_size_(buffer.size() * sizeof(typename TContainer::value_type)) {}

  void Seek(std::size_t offset) override { position_ = offset; }

  std::size_t Tell() override { return position_; }

  std::size_t Len() override { return buffer_size_; }

  /**
   * @brief Creates a view into a subrange of the buffer
   * @param offset Starting offset
   * @param size Size of the subview
   * @return A BufferView for the subrange
   */
  [[nodiscard]] BufferView SubView(std::size_t offset, std::size_t size) const {
    NNL_EXPECTS(offset + size <= buffer_size_);
    return BufferView(buffer_ptr_ + offset, size);
  }

  Buffer CopyBuf() const {
    std::vector<u8> copied_buf(buffer_size_);

    if (buffer_ptr_ != nullptr && buffer_size_ != 0) std::memcpy(copied_buf.data(), buffer_ptr_, buffer_size_);

    return copied_buf;
  }

  const u8& operator[](std::size_t index) const {
    NNL_EXPECTS_DBG(index < size());
    return buffer_ptr_[index];
  }

  friend bool operator==(const BufferView& lhs, const BufferView& rhs) {
    return (lhs.buffer_size_ == 0 && rhs.buffer_size_ == 0) ||
           (lhs.buffer_size_ == rhs.buffer_size_ &&
            std::memcmp(lhs.buffer_ptr_, rhs.buffer_ptr_, lhs.buffer_size_) == 0);
  }

  friend bool operator!=(const BufferView& lhs, const BufferView& rhs) { return !(lhs == rhs); }

  std::size_t size() const noexcept { return buffer_size_; }

  [[nodiscard]] std::size_t empty() const noexcept { return buffer_size_ == 0; }

  /**
   * @brief Gets pointer to the underlying buffer
   * @return Const pointer to buffer data
   */
  const u8* data() const noexcept { return buffer_ptr_; }

  const u8& at(std::size_t index) const {
    NNL_EXPECTS(index < buffer_size_);
    return buffer_ptr_[index];
  }

  const u8& front() const {
    NNL_EXPECTS_DBG(buffer_size_ > 0);
    return *begin();
  }

  const u8& back() const {
    NNL_EXPECTS_DBG(buffer_size_ > 0);
    return *rbegin();
  }

  const_iterator begin() const noexcept { return buffer_ptr_; }

  const_iterator end() const noexcept { return buffer_ptr_ + buffer_size_; }

  const_reverse_iterator rbegin() const noexcept { return std::make_reverse_iterator(end()); }

  const_reverse_iterator rend() const noexcept { return std::make_reverse_iterator(begin()); }

 protected:
  void ReadData(void* dst, std::size_t size) override {
    if (position_ + size > this->buffer_size_) {
      NNL_THROW(nnl::IOError(NNL_SRCTAG("Read exceeds buffer size")));
    }
    std::memcpy(dst, buffer_ptr_ + position_, size);
    position_ += size;
  }

 private:
  const u8* buffer_ptr_ = nullptr;
  std::size_t buffer_size_ = 0;
  std::size_t position_ = 0;
};

/**
 * @class nnl::BufferSpan
 * @brief Reader/Writer implementation for mutable memory buffers
 *
 * Provides read/write access to existing memory without taking ownership.
 * Suitable for modification of buffers or memory-mapped files.
 * Offers an interface similar to std::vector.
 *
 * @see nnl::BufferView
 */
class BufferSpan final : public Reader, public Writer {
 public:
  using value_type = u8;
  using iterator = value_type*;
  using reverse_iterator = std::reverse_iterator<iterator>;

  BufferSpan() noexcept {}
  /**
   * @brief Constructs a BufferSpan from a pointer/size pair.
   * @param buffer Pointer to the buffer.
   * @param size The _byte size_ of the buffer.
   */
  BufferSpan(void* buffer, std::size_t size) noexcept : buffer_ptr_(reinterpret_cast<u8*>(buffer)), buffer_size_(size) {
    NNL_EXPECTS_DBG(buffer != nullptr || size == 0);
  }

  /**
   * @brief Constructs a BufferView from a generic contiguous container.
   *
   * @param buffer Reference to the container to use as the source.
   */
  template <typename TContainer,
            typename = std::enable_if_t<std::is_trivially_copyable_v<typename TContainer::value_type>>>
  BufferSpan(TContainer& buffer) noexcept
      : buffer_ptr_(reinterpret_cast<u8*>(buffer.data())),
        buffer_size_(buffer.size() * sizeof(typename TContainer::value_type)) {}

  void Seek(std::size_t offset) override { position_ = offset; }

  std::size_t Tell() override { return position_; }

  std::size_t Len() override { return buffer_size_; }

  /**
   * @brief Clears the buffer with a specific value
   * @param val Value to fill with
   */
  void Clear(u8 val = 0) {
    if (buffer_ptr_ != nullptr) std::memset(buffer_ptr_, val, buffer_size_);
  }
  /**
   * @brief Creates a **read-only** view into a subrange
   * @param offset Starting offset
   * @param size Size of the subview
   * @return **BufferView** for the subrange
   * @see nnl::BufferSpan::SubSpan
   */
  [[nodiscard]] BufferView SubView(std::size_t offset, std::size_t size) const {
    NNL_EXPECTS(offset + size <= buffer_size_);

    return BufferView(buffer_ptr_ + offset, size);
  }

  /**
   * @brief Creates a **mutable** span into a subrange
   * @param offset Starting offset
   * @param size Size of the subspan
   * @return BufferSpan for the subrange
   * @see nnl::BufferSpan::SubView
   */
  [[nodiscard]] BufferSpan SubSpan(std::size_t offset, std::size_t size) {
    NNL_EXPECTS(offset + size <= buffer_size_);

    return BufferSpan(buffer_ptr_ + offset, size);
  }

  Buffer CopyBuf() const {
    std::vector<u8> copy(buffer_size_);
    if (buffer_ptr_ != nullptr && buffer_size_ != 0) std::memcpy(copy.data(), buffer_ptr_, buffer_size_);
    return copy;
  }

  /**
   * @brief Implicit conversion to BufferView
   */
  operator BufferView() const noexcept { return BufferView(buffer_ptr_, buffer_size_); }

  u8& operator[](std::size_t index) {
    NNL_EXPECTS_DBG(index < Len());
    return buffer_ptr_[index];
  }

  friend bool operator==(const BufferSpan& lhs, const BufferSpan& rhs) {
    return (lhs.buffer_size_ == 0 && rhs.buffer_size_ == 0) ||
           (lhs.buffer_size_ == rhs.buffer_size_ &&
            std::memcmp(lhs.buffer_ptr_, rhs.buffer_ptr_, lhs.buffer_size_) == 0);
  }

  friend bool operator!=(const BufferSpan& lhs, const BufferSpan& rhs) { return !(lhs == rhs); }

  std::size_t size() const noexcept { return buffer_size_; }

  [[nodiscard]] std::size_t empty() const noexcept { return buffer_size_ == 0; }
  /**
   * @brief Gets the pointer to the underlying buffer
   * @return Mutable pointer to buffer data
   */
  u8* data() noexcept { return buffer_ptr_; }

  u8& at(std::size_t index) {
    NNL_EXPECTS(index < buffer_size_);
    return buffer_ptr_[index];
  }

  u8& front() {
    NNL_EXPECTS_DBG(buffer_size_ > 0);
    return *begin();
  }

  u8& back() {
    NNL_EXPECTS_DBG(buffer_size_ > 0);
    return *rbegin();
  }

  iterator begin() noexcept { return buffer_ptr_; }

  iterator end() noexcept { return buffer_ptr_ + buffer_size_; }

  reverse_iterator rbegin() noexcept { return std::make_reverse_iterator(end()); }

  reverse_iterator rend() noexcept { return std::make_reverse_iterator(begin()); }

 protected:
  void ReadData(void* dst, std::size_t size) override {
    if (position_ + size > this->buffer_size_) {
      NNL_THROW(nnl::IOError(NNL_SRCTAG("Read exceeds buffer size")));
    }
    std::memcpy(dst, buffer_ptr_ + position_, size);
    position_ += size;
  }

  void WriteData(const void* src, std::size_t size) override {
    if (position_ + size > this->buffer_size_) {
      NNL_THROW(nnl::IOError(NNL_SRCTAG("Write exceeds buffer size")));
    }
    std::memcpy(buffer_ptr_ + position_, src, size);
    position_ += size;
  }

 private:
  u8* buffer_ptr_ = nullptr;
  std::size_t buffer_size_ = 0;
  std::size_t position_ = 0;
};
/**
 * @class nnl::BufferRW
 * @brief Reader/Writer implementation that writes to an internal growable
 * buffer
 *
 * Automatically manages memory allocation and grows the buffer as needed.
 * The written data can be extracted as an std::vector<u8> by a copy or move
 * operation.
 */
class BufferRW final : public Writer, public Reader {
 public:
  BufferRW() {}

  BufferRW(const BufferRW& buf) : buffer_(buf.buffer_), position_(buf.position_) {}

  BufferRW(BufferRW&& buf) noexcept : buffer_(std::move(buf.buffer_)), position_(buf.position_) { buf.position_ = 0; }

  BufferRW(const Buffer& buf) : buffer_(buf) {}

  BufferRW(Buffer&& buf) noexcept : buffer_(std::move(buf)) {}

  BufferRW& operator=(const BufferRW& buf) {
    if (this != &buf) {
      buffer_ = buf.buffer_;
      position_ = buf.position_;
    }
    return *this;
  }

  BufferRW& operator=(BufferRW&& buf) noexcept {
    if (this != &buf) {
      buffer_ = std::move(buf.buffer_);
      position_ = buf.position_;
      buf.position_ = 0;
    }
    return *this;
  }

  void Seek(std::size_t offset) override { position_ = offset; }

  std::size_t Tell() override { return position_; }

  std::size_t Len() override { return buffer_.size(); }
  /**
   * @brief Makes a copy of the data
   *
   * @return The copied buffer_
   */
  Buffer CopyBuf() const { return buffer_; }

  /**
   * @brief Transfers the ownership of the internal buffer_
   *
   * @return The internal buffer_
   */
  [[nodiscard]] Buffer ReleaseBuf() noexcept {
    position_ = 0;
    return std::move(buffer_);
  }

  operator Buffer() const { return buffer_; }

  /**
   * @brief Implicit conversion to std::vector<u8>&&
   *
   * This function can be used to _move_ the contents of the buffer_ to
   * std::vector<u8>.
   *
   * @return The moved buffer_
   */
  operator Buffer&&() && noexcept {
    position_ = 0;
    return std::move(buffer_);
  }

 protected:
  void WriteData(const void* src, std::size_t size) override {
    if (position_ + size > buffer_.size()) buffer_.resize(position_ + size);

    std::memcpy(buffer_.data() + position_, src, size);
    position_ += size;
  }

  void ReadData(void* dst, std::size_t size) override {
    if (position_ + size > this->buffer_.size()) {
      NNL_THROW(nnl::IOError(NNL_SRCTAG("Read exceeds buffer size")));
    }
    std::memcpy(dst, buffer_.data() + position_, size);
    position_ += size;
  }

 private:
  Buffer buffer_;
  std::size_t position_ = 0;
};

/** @} */

}  // namespace nnl
