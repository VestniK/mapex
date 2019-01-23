#pragma once

#include <cstdint>

struct point {
  uint32_t x = 0;
  uint32_t y = 0;
};

constexpr bool operator==(point l, point r) noexcept { return l.x == r.x && l.y == r.y; }
constexpr bool operator!=(point l, point r) noexcept { return l.x != r.x || l.y != r.y; }

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

constexpr uint64_t morton_code(point pt) noexcept { return interleave(pt.x) | (interleave(pt.y) << 1); }
constexpr point from_morton(uint64_t code) noexcept { return {deinterleave(code), deinterleave(code >> 1)}; }
