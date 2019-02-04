#pragma once

#include <vector>

#include <QtCore/QObject>

#include <portable_concurrency/future_fwd>

class network_thread;

class poidb : public QObject {
  Q_OBJECT
public:
  explicit poidb(QObject* parent = nullptr);

  void reload(network_thread& net);

signals:
  void updated();

private:
  struct poi_data {
    std::vector<uint64_t> advertized;
    std::vector<uint64_t> regular;
  };

private slots:
  void on_loaded();

private:
  static pc::future<poi_data> load_poi(network_thread& net);
  static pc::future<poi_data> fetch_poi_cache();
  static poi_data read_poi(const QString& path);

private:
  pc::future<poi_data> load_future_;
  poi_data data_;
};
