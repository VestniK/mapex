#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace varint {

template <typename InputIter, typename OutIter>
size_t pack(InputIter first, InputIter last, OutIter encoded) {
  size_t res = 0;
  for (; first != last; ++first, ++encoded, ++res) {
    uint64_t val = *first;
    for (; val > 0x7f; val >>= 7, ++encoded, ++res)
      *encoded = static_cast<char>(0x80 | (val & 0x7f));
    *encoded = static_cast<char>(val);
  }
  return res;
}

template <typename InputIter, typename OutIter>
void unpack(InputIter first, InputIter last, OutIter encoded) {
  uint64_t val = 0;
  unsigned shift = 0;
  for (; first != last; ++first) {
    uint64_t bt = static_cast<uint8_t>(*first);
    val |= (bt & 0x7f) << shift;
    shift += 7;
    if (!(bt & 0x80)) {
      *encoded = val;
      val = 0;
      shift = 0;
      ++encoded;
    }
  }
}

template <typename InputIter, typename OutIter>
InputIter unpack_n(InputIter first, InputIter last, size_t count, OutIter encoded) {
  uint64_t val = 0;
  unsigned shift = 0;
  for (; first != last && count > 0; ++first) {
    uint64_t bt = static_cast<uint8_t>(*first);
    val |= (bt & 0x7f) << shift;
    shift += 7;
    if (!(bt & 0x80)) {
      *encoded = val;
      val = 0;
      shift = 0;
      ++encoded;
      --count;
    }
  }
  return first;
}

} // namespace varint

namespace delta {

template <typename InputIt>
struct delta_iterator {
  uint64_t operator*() const noexcept(noexcept(*std::declval<InputIt>())) { return *it - prev; }
  delta_iterator& operator++() noexcept(noexcept(++std::declval<InputIt>()) && noexcept(*std::declval<InputIt>())) {
    prev = *it;
    ++it;
    return *this;
  }
  bool operator!=(delta_iterator rhs) const noexcept(noexcept(std::declval<InputIt>() != std::declval<InputIt>())) {
    return it != rhs.it;
  }

  InputIt it;
  uint64_t prev = 0;
};

template <typename OutIt>
struct delta_oiter {
  delta_oiter& operator*() noexcept { return *this; }
  delta_oiter& operator++() noexcept(noexcept(++std::declval<OutIt>())) {
    ++it;
    return *this;
  }
  delta_oiter& operator=(uint64_t val) noexcept(noexcept(*std::declval<OutIt>())) {
    accum += val;
    *it = accum;
    return *this;
  }

  OutIt it;
  uint64_t accum = 0;
};

template <typename InputIt, typename OutIt>
void pack(InputIt first, InputIt last, OutIt dest) {
  varint::pack(delta_iterator<InputIt>{first}, delta_iterator<InputIt>{last}, dest);
}

template <typename InputIt, typename OutIt>
void unpack(InputIt first, InputIt last, OutIt dest) {
  varint::unpack(first, last, delta_oiter<OutIt>{dest});
}

template <typename InputIt, typename OutIt>
InputIt unpack_n(InputIt first, InputIt last, size_t count, OutIt dest) {
  return varint::unpack_n(first, last, count, delta_oiter<OutIt>{dest});
}

} // namespace delta
