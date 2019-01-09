#include <cmath>
#include <utility>

#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>

#include <portable_concurrency/future>

#include <mapex/tile_widget.hpp>
#include <mapex/tile_loader.hpp>

namespace {

constexpr double deg2rad(double deg) noexcept {
  return deg*M_PI/180.;
}

constexpr int tile_pixel_size = 256;
constexpr int max_z_level = 16;

QPointF project(geo_point point) noexcept {
  const double x = (static_cast<double>(point.lon) + 180.)/360.;

  const double lat_rad = deg2rad(static_cast<double>(point.lat));
  const double y = (1. - std::log(std::tan(lat_rad) + 1./std::cos(lat_rad))/M_PI)/2.;
  return {x, y};
}

QPoint floor(QPointF point) noexcept {
  return {static_cast<int>(std::floor(point.x())), static_cast<int>(std::floor(point.y()))};
}

constexpr QPoint int_div(QPoint point, int denom) noexcept {
  // `point/denom` will cast everyting to `qreal` then divide and then `qRound` the result
  return {point.x()/denom, point.y()/denom};
}

constexpr QPoint min(QPoint a, QPoint b) noexcept {
  return {std::min(a.x(), b.x()), std::min(a.y(), b.y())};
}

constexpr QPoint max(QPoint a, QPoint b) noexcept {
  return {std::max(a.x(), b.x()), std::max(a.y(), b.y())};
}

} // namespace

tile_widget::tile_widget(geo_point center, int z_level, QWidget* parent):
  QWidget{parent},
  projected_center_{project(center)},
  z_level_{z_level}
{}

void tile_widget::center_at(geo_point pos) {
  projected_center_ = project(pos);
  update();
}

void tile_widget::paintEvent(QPaintEvent* event) {
  const int tiles_coord_range = (1<<z_level_);
  const auto projected_top_left =
    floor(tile_pixel_size*tiles_coord_range*projected_center_) - (rect().center() - rect().topLeft())
  ;
  const QRect vp_projection = rect().translated(projected_top_left);
  const QPoint min_tile = max(int_div(vp_projection.topLeft(), tile_pixel_size), {0, 0});
  const QPoint max_tile = min(
    int_div(vp_projection.bottomRight(), tile_pixel_size) + QPoint{1, 1},
    {tiles_coord_range, tiles_coord_range}
  );
  const QSize tile_size{tile_pixel_size, tile_pixel_size};

  QPainter painter(this);
  for (int tx = min_tile.x(); tx < max_tile.x(); ++tx) {
    for (int ty = min_tile.y(); ty < max_tile.y(); ++ty) {
      const auto tile = QRect{QPoint{tx, ty}*tile_pixel_size, tile_size}.translated(-projected_top_left);
      auto it = images_.find({tx, ty, z_level_});
      if (it == images_.end())
        it = images_.insert({{tx, ty, z_level_}, load_tile(nm_, tx, ty, z_level_)}).first;
      if (!it->second.is_ready())
        continue;
      painter.drawImage(tile, it->second.get());
    }
  }
  event->accept();
}

void tile_widget::wheelEvent(QWheelEvent* event) {
  if (event->orientation() != Qt::Vertical)
    return QWidget::wheelEvent(event);
  wheel_accum_ += event->angleDelta().y();
  event->accept();
  // event->angleDelta().y() unit is 1/8 of degree and most common wheel step is 15 degrees
  constexpr int wheel_step = 15*8;
  if (wheel_accum_ < wheel_step && wheel_accum_ > -wheel_step)
    return;
  const int old_z_level = std::exchange(z_level_, std::clamp(z_level_ + wheel_accum_/wheel_step, 0, max_z_level));
  wheel_accum_ %= wheel_step;

  const QPointF shift = (event->posF() - rect().center())*((1<<z_level_)/(1<<old_z_level) - 1.);
  projected_center_ -= shift/(tile_pixel_size*(1<<z_level_));
  projected_center_.rx() = std::clamp(projected_center_.x(), 0., 1.);
  projected_center_.ry() = std::clamp(projected_center_.y(), 0., 1.);

  update();
}

void tile_widget::mousePressEvent(QMouseEvent* event) {
  if (std::exchange(last_mouse_move_pos_, std::nullopt))
    return event->accept();

  if (event->buttons() != Qt::LeftButton)
    return QWidget::mouseMoveEvent(event);

  last_mouse_move_pos_ = event->pos();
  event->accept();
}

void tile_widget::mouseReleaseEvent(QMouseEvent* event) {
  if (std::exchange(last_mouse_move_pos_, std::nullopt))
    return event->accept();
  return QWidget::mouseMoveEvent(event);
}

void tile_widget::mouseMoveEvent(QMouseEvent* event) {
  if (!last_mouse_move_pos_)
    return QWidget::mouseMoveEvent(event);
  const QPointF shift = event->pos() - std::exchange(*last_mouse_move_pos_, event->pos());
  projected_center_ -= shift/(tile_pixel_size*(1<<z_level_));
  projected_center_.rx() = std::clamp(projected_center_.x(), 0., 1.);
  projected_center_.ry() = std::clamp(projected_center_.y(), 0., 1.);
  update();
  event->accept();
}
