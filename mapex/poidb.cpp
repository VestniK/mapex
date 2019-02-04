#include <array>
#include <fstream>

#include <QtCore/QDir>
#include <QtCore/QMetaMethod>
#include <QtCore/QSaveFile>
#include <QtCore/QStandardPaths>
#include <QtCore/QThreadPool>

#include <QtNetwork/QNetworkReply>

#include <portable_concurrency/future>

#include <mapex/deltapack.hpp>
#include <mapex/network_thread.hpp>
#include <mapex/poidb.hpp>

namespace {

QString poi_cache_path() {
  const auto cache_dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
  if (!QFileInfo::exists(cache_dir) && !QDir{cache_dir}.mkpath(".")) {
    throw std::system_error{
        std::make_error_code(std::errc::no_such_file_or_directory), "create cache dir"}; // TODO: better error
  }
  return QDir{cache_dir}.filePath("poi.bin");
}

} // namespace

poidb::poidb(QObject* parent) : QObject(parent) {}

void poidb::reload(network_thread& net) {
  std::array<pc::future<poi_data>, 2> futures = {load_poi(net).detach(), fetch_poi_cache()};
  load_future_ = pc::when_any(futures.begin(), futures.end())
                     .next([](pc::when_any_result<std::vector<pc::future<poi_data>>> res) {
                       poi_data data = res.futures[res.index].get(); // TODO: handle network errors here
                       if (res.index == 1 && data.regular.empty() && data.advertized.empty())
                         return std::move(res.futures[0]);
                       return pc::make_ready_future(std::move(data));
                     })
                     .then([this](auto f) {
                       QMetaObject::invokeMethod(this, &poidb::on_loaded, Qt::QueuedConnection);
                       return f;
                     });
}

void poidb::on_loaded() {
  if (!load_future_.valid() || !load_future_.is_ready())
    return;
  data_ = load_future_.get();
  emit updated();
}

pc::future<poidb::poi_data> poidb::load_poi(network_thread& net) {
  return pc::async(net.executor(), [&net]() {
    return send_request(net.network_manager(), QUrl("https://raw.githubusercontent.com/VestniK/mapex/master/poi.bin"))
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
  });
}

pc::future<poidb::poi_data> poidb::fetch_poi_cache() {
  return pc::async(QThreadPool::globalInstance(), read_poi, poi_cache_path());
}

poidb::poi_data poidb::read_poi(const QString& path) {
  poidb::poi_data res;
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
