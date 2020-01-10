#ifndef ALT_INTEGRATION_WRITE_STREAM_HPP
#define ALT_INTEGRATION_WRITE_STREAM_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace VeriBlock {

  /**
   * Binary writer that is useful dugin binary serialization.
   */
  class WriteStream {
   public:
    WriteStream() = default;

    explicit WriteStream(size_t size);

    void write(const void *buf, size_t size);

    template <typename T,
              typename =
                  typename std::enable_if<sizeof(typename T::value_type)>::type>
    void write(const T &t) {
      write(t.data(), t.size());
    }

    template <
        typename T,
        typename = typename std::enable_if<std::is_integral<T>::value>::type>
    void writeBE(T num) {
      for (size_t i = 0, shift = 0; i < sizeof(T); i++, shift += 8) {
        m_data.push_back((num >> shift) & 0xffu);
      }
    }

    template <
        typename T,
        typename = typename std::enable_if<std::is_integral<T>::value>::type>
    void writeLE(T num) {
      for (size_t i = 0, shift = (sizeof(T) - 1) * 8; i < sizeof(T);
           i++, shift -= 8) {
        m_data.push_back((num >> shift) & 0xffu);
      }
    }

    const std::vector<uint8_t> &data() const noexcept;

   private:
    std::vector<uint8_t> m_data;
  };

}  // namespace VeriBlock

#endif  // ALT_INTEGRATION_WRITE_STREAM_HPP