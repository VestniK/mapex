#include <array>
#include <fstream>

#include <QtCore/QDir>
#include <QtCore/QMetaMethod>
#include <QtCore/QRectF>
#include <QtCore/QSaveFile>
#include <QtCore/QStandardPaths>
#include <QtCore/QThreadPool>

#include <QtNetwork/QNetworkReply>

#include <portable_concurrency/future>

#include <mapex/deltapack.hpp>
#include <mapex/network_thread.hpp>
#include <mapex/poidb.hpp>

struct poi_data {
  std::vector<uint64_t> advertized;
  std::vector<uint64_t> regular;
};

namespace {

QString poi_cache_path() {
  const auto cache_dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
  if (!QFileInfo::exists(cache_dir) && !QDir{cache_dir}.mkpath(".")) {
    throw std::system_error{
        std::make_error_code(std::errc::no_such_file_or_directory), "create cache dir"}; // TODO: better error
  }
  return QDir{cache_dir}.filePath("poi.bin");
}

poi_data read_poi(const QString& path) {
  poi_data res;
  if (!QFileInfo::exists(path))
    return res;

  std::filebuf in;
  if (!in.open(path.toLocal8Bit().constData(), std::ios_base::in))
    throw std::system_error{errno, std::system_category(), "open: " + path.toStdString()};

  uint64_t adv_count;
  auto it = varint::unpack_n(std::istreambuf_iterator{&in}, {}, 1, &adv_count);
  it = delta::unpack_n(it, {}, adv_count, std::back_inserter(res.advertized));
  delta::unpack(it, {}, std::back_inserter(res.regular));
  return res;
}

pc::future<poi_data> load_poi(network_thread& net) {
  return net.send_request(QUrl("https://raw.githubusercontent.com/VestniK/mapex/master/poi.bin"))
      .next([](std::unique_ptr<QNetworkReply> reply) {
        QSaveFile sf{poi_cache_path()};
        if (!sf.open(QIODevice::WriteOnly)) {
          throw std::system_error{
              std::make_error_code(std::errc::no_such_file_or_directory), "create cache file"}; // TODO: better error
        }
        const QByteArray content = reply->readAll();
        sf.write(content);
        if (!sf.commit())
          throw std::system_error{std::make_error_code(std::errc::io_error), "save cache file"}; // TODO: better error

        return pc::async(QThreadPool::globalInstance(), read_poi, poi_cache_path());
      });
}

pc::future<poi_data> fetch_poi_cache() { return pc::async(QThreadPool::globalInstance(), read_poi, poi_cache_path()); }

} // namespace

poidb::poidb(QObject* parent) : QObject(parent) {}

void poidb::reload(network_thread& net) {
  auto notify = [this](pc::future<poi_data> f) {
    QMetaObject::invokeMethod(this, &poidb::on_loaded, Qt::QueuedConnection);
    return f;
  };
  std::array<pc::future<poi_data>, 2> futures = {load_poi(net).then(notify).detach(), fetch_poi_cache()};
  load_future_ = pc::when_any(futures.begin(), futures.end())
                     .next([](pc::when_any_result<std::vector<pc::future<poi_data>>> res) {
                       poi_data data = res.futures[res.index].get(); // TODO: handle network errors here
                       if (res.index == 1 && data.regular.empty() && data.advertized.empty())
                         return std::move(res.futures[0]);
                       return pc::make_ready_future(std::move(data));
                     })
                     .then(notify);
}

pc::future<std::vector<marker>> poidb::generalize(const QRectF& viewport, int z_level) const {
  std::array<pc::future<std::vector<marker>>, 2> futures = {
      pc::async(QThreadPool::globalInstance(),
          [] {
            return std::vector<marker>{{marker{QPointF{0.7303155547905815, 0.31611348163573294}}}};
          }),
      pc::async(QThreadPool::globalInstance(), [] { return std::vector<marker>{}; })};
  return pc::when_all(std::make_move_iterator(futures.begin()), std::make_move_iterator(futures.end()))
      .next([](std::vector<pc::future<std::vector<marker>>> results) { return results[0].get(); });
}

void poidb::on_loaded() {
  if (!load_future_.valid() || !load_future_.is_ready())
    return;
  data_ = std::make_shared<poi_data>(load_future_.get());
  emit updated();
}
