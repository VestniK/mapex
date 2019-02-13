#include <cmath>
#include <utility>

#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>

#include <portable_concurrency/future>

#include <mapex/network_thread.hpp>
#include <mapex/tile_loader.hpp>
#include <mapex/tile_widget.hpp>

namespace {

constexpr double deg2rad(double deg) noexcept { return deg * M_PI / 180.; }

constexpr int tile_pixel_size = 256;
constexpr int max_z_level = 16;
constexpr QSize tile_size{tile_pixel_size, tile_pixel_size};
constexpr QSize poi_icon_size{24, 24};

QPointF project(geo_point point) noexcept {
  const double x = (static_cast<double>(point.lon) + 180.) / 360.;

  const double lat_rad = deg2rad(static_cast<double>(point.lat));
  const double y = (1. - std::log(std::tan(lat_rad) + 1. / std::cos(lat_rad)) / M_PI) / 2.;
  return {x, y};
}

QPoint floor(QPointF point) noexcept {
  return {static_cast<int>(std::floor(point.x())), static_cast<int>(std::floor(point.y()))};
}

constexpr QPointF squre_clamp(QPointF point, qreal min, qreal max) noexcept {
  return {std::clamp(point.x(), min, max), std::clamp(point.y(), min, max)};
}

constexpr QPoint int_div(QPoint point, int denom) noexcept {
  // `point/denom` will cast everyting to `qreal` then divide and then `qRound` the result
  return {point.x() / denom, point.y() / denom};
}

constexpr QPoint min(QPoint a, QPoint b) noexcept { return {std::min(a.x(), b.x()), std::min(a.y(), b.y())}; }

constexpr QPoint max(QPoint a, QPoint b) noexcept { return {std::max(a.x(), b.x()), std::max(a.y(), b.y())}; }

} // namespace

tile_widget::tile_widget(geo_point center, int z_level, network_thread* net, QWidget* parent)
    : QWidget{parent}, net_{net}, icons_({QImage{"icons:multi-poi.png"}, QImage{"icons:multi-adw.png"},
                                      QImage{"icons:single-poi.png"}, QImage{"icons:single-adw.png"}}),
      projected_center_{project(center)}, z_level_{z_level} {
  connect(&poi_, &poidb::updated, this, &tile_widget::on_viewport_change);
  on_viewport_change();
}

void tile_widget::center_at(geo_point pos) {
  projected_center_ = project(pos);
  on_viewport_change();
}

void tile_widget::set_poi_visible(bool val) {
  if (poi_visible_ == val)
    return;
  poi_visible_ = val;
  if (poi_visible_)
    poi_.reload(*net_);
  else {
    current_markers_area_ = {};
    markers_.clear();
    markers_future_ = {};
    update();
  }
}

void tile_widget::paintEvent(QPaintEvent* event) {
  check_finished_tasks();

  const int tiles_coord_range = (1 << z_level_);
  const auto projected_top_left =
      floor(tile_pixel_size * tiles_coord_range * projected_center_) - (rect().center() - rect().topLeft());
  QPainter painter(this);
  for (const auto& [tile, image] : images_) {
    const auto tile_rect = QRect{QPoint{tile.x, tile.y} * tile_pixel_size, tile_size}.translated(-projected_top_left);
    painter.drawImage(tile_rect, image);
  }
  painter.setPen(QPen{Qt::blue, 3});
  for (const marker& poi : markers_) {
    const auto pin_pt = floor(poi.point * tile_pixel_size * tiles_coord_range) - projected_top_left;
    QRect pin_rect{{}, poi_icon_size};
    pin_rect.moveCenter(pin_pt);
    painter.drawImage(pin_rect, icons_[(poi.poi_count == 1 ? 2 : 0) | (poi.has_advertizers ? 1 : 0)]);
  }
  event->accept();
}

void tile_widget::resizeEvent(QResizeEvent* event) {
  on_viewport_change();
  event->accept();
}

void tile_widget::wheelEvent(QWheelEvent* event) {
  if (event->orientation() != Qt::Vertical)
    return QWidget::wheelEvent(event);
  wheel_accum_ += event->angleDelta().y();
  event->accept();
  // event->angleDelta().y() unit is 1/8 of degree and most common wheel step is 15 degrees
  constexpr int wheel_step = 15 * 8;
  if (wheel_accum_ < wheel_step && wheel_accum_ > -wheel_step)
    return;
  const int old_z_level = std::exchange(z_level_, std::clamp(z_level_ + wheel_accum_ / wheel_step, 0, max_z_level));
  wheel_accum_ %= wheel_step;

  const QPointF shift = (event->posF() - rect().center()) / tile_pixel_size;
  projected_center_ += shift * ((1 << z_level_) - (1 << old_z_level)) / (1 << (old_z_level + z_level_));
  projected_center_ = squre_clamp(projected_center_, 0., 1.);
  current_markers_area_ = {}; // invalidate markers area to force generalization request
  on_viewport_change();
}

void tile_widget::mousePressEvent(QMouseEvent* event) {
  if (std::exchange(last_mouse_move_pos_, std::nullopt))
    return event->accept();

  if (event->buttons() != Qt::LeftButton)
    return QWidget::mousePressEvent(event);

  last_mouse_move_pos_ = event->pos();
  event->accept();
}

void tile_widget::mouseReleaseEvent(QMouseEvent* event) {
  if (!std::exchange(last_mouse_move_pos_, std::nullopt))
    return QWidget::mouseReleaseEvent(event);
  event->accept();
}

void tile_widget::mouseMoveEvent(QMouseEvent* event) {
  if (!last_mouse_move_pos_)
    return QWidget::mouseMoveEvent(event);
  const QPointF shift = event->pos() - std::exchange(*last_mouse_move_pos_, event->pos());
  projected_center_ -= shift / (tile_pixel_size * (1 << z_level_));
  projected_center_ = squre_clamp(projected_center_, 0., 1.);
  on_viewport_change();
  event->accept();
}

void tile_widget::on_viewport_change() {
  const int tiles_coord_range = (1 << z_level_);
  if (poi_visible_) {
    QRectF vp_rect{{}, QSizeF{rect().size()} / (tile_pixel_size * tiles_coord_range)};
    vp_rect.moveCenter(projected_center_);
    if (!current_markers_area_.contains(vp_rect)) {
      current_markers_area_.setSize(2 * vp_rect.size());
      current_markers_area_.moveCenter(projected_center_);
      markers_future_ = poi_.generalize(current_markers_area_, z_level_).then(executor(), [this](auto f) {
        update();
        return f;
      });
    }
  }
  const auto projected_top_left =
      floor(tile_pixel_size * tiles_coord_range * projected_center_) - (rect().center() - rect().topLeft());
  const QRect vp_projection = rect().translated(projected_top_left);
  const QPoint min_tile = max(int_div(vp_projection.topLeft(), tile_pixel_size), {0, 0});
  const QPoint max_tile =
      min(int_div(vp_projection.bottomRight(), tile_pixel_size) + QPoint{1, 1}, {tiles_coord_range, tiles_coord_range});

  std::map<tile_id, QImage> new_images;
  std::map<tile_id, pc::future<QImage>> new_tasks;
  for (int tx = min_tile.x(); tx < max_tile.x(); ++tx) {
    for (int ty = min_tile.y(); ty < max_tile.y(); ++ty) {
      tile_id tid = {tx, ty, z_level_};
      if (auto node = images_.extract(tid)) {
        new_images.insert(std::move(node));
        continue;
      }

      if (auto node = tasks_.extract(tid)) {
        new_tasks.insert(std::move(node));
        continue;
      }

      new_tasks[tid] = load_tile(*net_, tid.x, tid.y, tid.z_level).then(executor(), [this](auto f) {
        update();
        return f;
      });
    }
  }
  std::swap(new_images, images_);
  std::swap(new_tasks, tasks_);
  update();
}

void tile_widget::check_finished_tasks() {
  for (auto it = tasks_.begin(); it != tasks_.end();) {
    if (!it->second.is_ready()) {
      ++it;
      continue;
    }
    try {
      images_[it->first] = it->second.get();
    } catch (network_error err) { // TODO: network_error
      qWarning("Network error: %s", err.what());
    }
    it = tasks_.erase(it);
  }

  if (markers_future_.valid() && markers_future_.is_ready())
    markers_ = markers_future_.get();
}
