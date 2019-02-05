#pragma once

#include <memory>

#include <QtCore/QObject>
#include <QtCore/QPointF>

#include <portable_concurrency/future_fwd>

class network_thread;
struct poi_data;

struct marker {
  QPointF point;
  int poi_count = 1;
  bool has_advertizers = false;
};

class poidb : public QObject {
  Q_OBJECT
public:
  explicit poidb(QObject* parent = nullptr);

  void reload(network_thread& net);

  [[nodiscard]] pc::future<std::vector<marker>> generalize(const QRectF& viewport, int z_level) const;

signals:
  void updated();

private slots:
  void on_loaded();

private:
  pc::future<poi_data> load_future_;
  std::shared_ptr<const poi_data> data_;
};
