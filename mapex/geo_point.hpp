#pragma once

template <typename Rep, auto Unit>
class unit {
public:
  constexpr unit() noexcept = default;
  constexpr explicit unit(Rep value) noexcept : value_{value} {}

  constexpr explicit operator Rep() const noexcept { return value_; }

private:
  Rep value_ = {};
};

template <typename Rep, auto Unit>
constexpr unit<Rep, Unit> operator+(unit<Rep, Unit> l, unit<Rep, Unit> r) noexcept {
  return unit<Rep, Unit>{static_cast<Rep>(l) + static_cast<Rep>(r)};
}

template <typename Rep, auto Unit>
constexpr unit<Rep, Unit> operator-(unit<Rep, Unit> l, unit<Rep, Unit> r) noexcept {
  return unit<Rep, Unit>{static_cast<Rep>(l) - static_cast<Rep>(r)};
}

enum class geo_units { longiture, lattitude };
using longitude = unit<double, geo_units::longiture>;
constexpr longitude operator"" _lon(long double val) noexcept { return longitude{static_cast<double>(val)}; }
using lattitude = unit<double, geo_units::lattitude>;
constexpr lattitude operator"" _lat(long double val) noexcept { return lattitude{static_cast<double>(val)}; }

struct geo_point {
  longitude lon;
  lattitude lat;
};
