#pragma once

#include <cassert>
#include <cstdint>
#include <numeric>

struct point {
  uint32_t x = 0;
  uint32_t y = 0;
};

constexpr bool operator==(point l, point r) noexcept { return l.x == r.x && l.y == r.y; }
constexpr bool operator!=(point l, point r) noexcept { return l.x != r.x || l.y != r.y; }

namespace morton {

namespace detial {

constexpr uint64_t S[] = {16, 8, 4, 2, 1};
constexpr uint64_t M[] = {
    0x0000'ffff'0000'ffff, 0x00ff'00ff'00ff'00ff, 0x0f0f'0f0f'0f0f'0f0f, 0x3333'3333'3333'3333, 0x5555'5555'5555'5555};

} // namespace detial

constexpr uint64_t interleave(uint32_t val) noexcept {
  using detial::M;
  using detial::S;
  uint64_t r = val;
  r = (r | (r << S[0])) & M[0];
  r = (r | (r << S[1])) & M[1];
  r = (r | (r << S[2])) & M[2];
  r = (r | (r << S[3])) & M[3];
  r = (r | (r << S[4])) & M[4];
  return r;
}

constexpr uint32_t deinterleave(uint64_t val) noexcept {
  using detial::M;
  using detial::S;
  val = val & M[4];
  val = (val | (val >> S[4])) & M[3];
  val = (val | (val >> S[3])) & M[2];
  val = (val | (val >> S[2])) & M[1];
  val = (val | (val >> S[1])) & M[0];
  val = (val | (val >> S[0])) & 0x0000'0000'ffff'ffff;
  return static_cast<uint32_t>(val);
}

constexpr uint64_t code(point pt) noexcept { return interleave(pt.x) | (interleave(pt.y) << 1); }
constexpr point decode(uint64_t code) noexcept { return {deinterleave(code), deinterleave(code >> 1)}; }

namespace detail {

constexpr uint64_t load_masks[] = {interleave(uint32_t{1} << 31) << 1, interleave(~(uint32_t{1} << 31)) << 1};
constexpr uint64_t untoched_bits_mask = interleave(~uint32_t{0}) << 1;
constexpr unsigned u64_bits_count = 64;

// Operation actually stores certain bits in the current axis bits but `load` name is used in bigmin/litmax algorythms
// explanation in the original work introducing them: Tropf, H.; Herzog, H. (1981), "Multidimensional Range Search in
// Dynamically Balanced Trees", Angewandte Informatik, 2: 71–77.
constexpr uint64_t load(uint64_t mask, unsigned bit_pos, uint64_t code) noexcept {
  mask = mask >> (u64_bits_count - 1 - bit_pos);
  const uint64_t zero_bits_reset = code & ((untoched_bits_mask >> (bit_pos % 2)) | (~uint64_t{1} << bit_pos) | mask);
  return zero_bits_reset | mask;
}

} // namespace detail

constexpr uint64_t bigmin(uint64_t division_point, uint64_t min, uint64_t max) noexcept {
  assert(division_point < max);
  assert(min <= max);
  // min and max must be codes of rect corners with minimal coordinate values and maximal coordinate values.
  assert(decode(min).x <= decode(max).x);
  assert(decode(min).y <= decode(max).y);
  uint64_t res = max;
  for (int bit_pos = detail::u64_bits_count - 1; bit_pos >= 0; --bit_pos) {
    const uint64_t examined_bits =
        (((division_point >> bit_pos) & 1) << 2) | (((min >> bit_pos) & 1) << 1) | ((max >> bit_pos) & 1);
    switch (examined_bits) {
    case 0b000:
    case 0b111:
      break;
    case 0b001:
      res = detail::load(detail::load_masks[0], bit_pos, min);
      max = detail::load(detail::load_masks[1], bit_pos, max);
      break;
    case 0b101:
      min = detail::load(detail::load_masks[0], bit_pos, min);
      break;
    case 0b011:
      return min;
    case 0b100:
      return res;
    }
  }
  return res;
}

} // namespace morton
