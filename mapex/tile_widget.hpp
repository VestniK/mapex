#pragma once

#include <optional>

#include <QWidget>

template<typename Rep, auto Unit>
class unit {
public:
  constexpr unit() noexcept = default;
  constexpr explicit unit(Rep value) noexcept: value_{value} {}

  constexpr explicit operator Rep () const noexcept {return value_;}

private:
  Rep value_ = {};
};

template<typename Rep, auto Unit>
constexpr unit<Rep, Unit> operator+ (unit<Rep, Unit> l, unit<Rep, Unit> r) noexcept {
  return unit<Rep, Unit>{static_cast<Rep>(l) + static_cast<Rep>(r)};
}

template<typename Rep, auto Unit>
constexpr unit<Rep, Unit> operator- (unit<Rep, Unit> l, unit<Rep, Unit> r) noexcept {
  return unit<Rep, Unit>{static_cast<Rep>(l) - static_cast<Rep>(r)};
}

enum class geo_units {longiture, lattitude};
using longitude = unit<double, geo_units::longiture>;
constexpr longitude operator"" _lon(long double val) noexcept {return longitude{static_cast<double>(val)};}
using lattitude = unit<double, geo_units::lattitude>;
constexpr lattitude operator"" _lat(long double val) noexcept {return lattitude{static_cast<double>(val)};}

struct geo_point {
  longitude lon;
  lattitude lat;
};

class tile_widget final: public QWidget {
  Q_OBJECT
  Q_PROPERTY(int z_level READ z_level WRITE set_z_level)
public:
  tile_widget(geo_point center, int z_level, QWidget* parent = nullptr);

  void center_at(geo_point val);

  int z_level() const noexcept {return z_level_;}
  void set_z_level(int val) {z_level_ = val; update();}

protected:
  void paintEvent(QPaintEvent* event) override;
  void wheelEvent(QWheelEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;

private:
  QPointF projected_center_;
  int z_level_ = 12;
  int wheel_accum_ = 0;
  std::optional<QPoint> last_mouse_move_pos_;
};
