#pragma once

#include <map>
#include <optional>
#include <tuple>

#include <QtWidgets/QWidget>

#include <portable_concurrency/future_fwd>

#include <mapex/executors.hpp>
#include <mapex/geo_point.hpp>
#include <mapex/poidb.hpp>

class network_thread;

struct tile_id {
  int x = 0;
  int y = 0;
  int z_level = 0;

  bool operator<(const tile_id& rhs) const noexcept {
    return std::tie(z_level, x, y) < std::tie(rhs.z_level, rhs.x, rhs.y);
  }
};

class tile_widget final : public QWidget {
  Q_OBJECT
  Q_PROPERTY(int z_level READ z_level WRITE set_z_level)
  Q_PROPERTY(bool poi_visible READ is_poi_visible WRITE set_poi_visible)
public:
  using executor_type = QObject*;

  tile_widget(geo_point center, int z_level, network_thread* net, QWidget* parent = nullptr);

  void center_at(geo_point val);

  int z_level() const noexcept { return z_level_; }
  void set_z_level(int val) {
    z_level_ = val;
    update();
  }

  bool is_poi_visible() const noexcept { return poi_visible_; }

  executor_type executor() noexcept { return this; }

public slots:
  void set_poi_visible(bool val);

protected:
  void paintEvent(QPaintEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;
  void wheelEvent(QWheelEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;

private slots:
  void on_viewport_change();

private:
  void check_finished_tasks();

private:
  network_thread* net_ = nullptr;

  poidb poi_;
  QRectF current_markers_area_;
  pc::future<std::vector<marker>> markers_future_;
  std::vector<marker> markers_;

  std::map<tile_id, QImage> images_;
  std::map<tile_id, pc::future<QImage>> tasks_;

  QPointF projected_center_;
  int z_level_ = 12;

  int wheel_accum_ = 0;
  std::optional<QPoint> last_mouse_move_pos_;
  bool poi_visible_ = false;
};
