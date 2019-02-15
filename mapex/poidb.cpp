#include <array>
#include <cmath>
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
#include <mapex/morton_code.hpp>
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

struct point_group {
  uint64_t morton_code;
  int count;
};

point pointf_to_point(QPointF pt) noexcept {
  assert(pt.x() < 1.0 && pt.x() >= 0.0);
  assert(pt.y() < 1.0 && pt.y() >= 0.0);
  return point{static_cast<uint32_t>(std::numeric_limits<uint32_t>::max() * pt.x()),
      static_cast<uint32_t>(std::numeric_limits<uint32_t>::max() * pt.y())};
}

uint64_t pointf_to_morton(QPointF pt) noexcept { return morton::code(pointf_to_point(pt)); }

QPointF pointf_from_morton(uint64_t code) noexcept {
  const auto pt = morton::decode(code);
  const double max_coord = std::pow(2., 32.);
  return {pt.x / max_coord, pt.y / max_coord};
}

constexpr unsigned cell_pixel_size_log2 = 5;
constexpr unsigned tile_pixel_size_log2 = 8;
constexpr unsigned world_coord_range_log2 = 32;

std::vector<point_group> generalize(
    const std::vector<uint64_t>& points, uint64_t vp_min, uint64_t vp_max, int z_level) {

  const int cell_coord_range_log2 = world_coord_range_log2 - (tile_pixel_size_log2 - cell_pixel_size_log2 + z_level);
  const uint64_t cell_mask = ~uint64_t{0} << (2 * cell_coord_range_log2);
  const uint64_t cell_step = uint64_t{1} << (2 * cell_coord_range_log2);
  std::vector<point_group> res;
  auto first = std::lower_bound(points.begin(), points.end(), vp_min);
  const auto last = std::upper_bound(points.begin(), points.end(), vp_max);
  while (first != last) {
    if (!is_in_rect(morton::decode(*first), morton::decode(vp_min), morton::decode(vp_max)))
      first = std::lower_bound(first, last, morton::bigmin(*first, vp_min, vp_max));

    uint64_t x_sum = 0;
    uint64_t y_sum = 0;
    int count = 0;
    const uint64_t next_cell_start = ((*first) & cell_mask) + cell_step;
    for (; first != last && *first < next_cell_start; ++first) {
      const point pt = morton::decode(*first);
      x_sum += pt.x;
      y_sum += pt.y;
      ++count;
    }
    if (count > 0) {
      res.push_back(point_group{
          morton::code({static_cast<uint32_t>(x_sum / count), static_cast<uint32_t>(y_sum / count)}), count});
    }
  }
  return res;
}

constexpr point aligned_point(point pt, unsigned bit_alignment) noexcept {
  const uint32_t mask = ~uint32_t{0} << bit_alignment;
  return {pt.x & mask, pt.y & mask};
}

QPointF abs(QPointF pt) noexcept { return {std::abs(pt.x()), std::abs(pt.y())}; }

bool marker_overlaps(const QPointF& lhs, const QPointF& rhs, uint32_t min_dist) noexcept {
  const point diff = pointf_to_point(abs(rhs - lhs));
  return diff.x < min_dist || diff.y < min_dist;
}

marker merge_marker(marker lhs, marker rhs) noexcept {
  marker res{(lhs.poi_count * lhs.point + rhs.poi_count * rhs.point) / (lhs.poi_count + rhs.poi_count),
      (lhs.poi_count + rhs.poi_count), lhs.has_advertizers || rhs.has_advertizers};
  return res;
}

void merge_marker(std::vector<marker>& markers, marker item, uint32_t min_dist) noexcept {
  for (auto it = markers.begin(); it != markers.end();) {
    if (!marker_overlaps(it->point, item.point, min_dist)) {
      ++it;
      continue;
    }
    item = merge_marker(item, *it);
    it = markers.erase(it);
    return merge_marker(markers, item, min_dist);
  }
  markers.push_back(item);
}

std::vector<marker> merge_generalizations(
    std::vector<point_group> ads, std::vector<point_group> poi, uint64_t vp_min, uint64_t vp_max, int z_level) {
  std::vector<marker> res;
  std::vector<marker> box_markers;
  const unsigned group_bit_alignment =
      world_coord_range_log2 - (tile_pixel_size_log2 - (cell_pixel_size_log2 + 1) + z_level);
  const point vp_min_pt = aligned_point(morton::decode(vp_min), group_bit_alignment);
  const point vp_max_pt = morton::decode(vp_max);

  const uint32_t box_side = (uint32_t{1} << group_bit_alignment);
  for (point box_min = vp_min_pt; box_min.x < vp_max_pt.x; box_min.x += box_side / 2) { // TODO: handle overflows
    for (box_min.y = vp_min_pt.y; box_min.y < vp_max_pt.y; box_min.y += box_side / 2) { // TODO: handle overflows
      const point box_max = box_min + point{box_side, box_side};                        // TODO: handle overflows

      for (auto it = res.begin(); it != res.end();) {
        if (!is_in_rect(pointf_to_point(it->point), box_min, box_max)) {
          ++it;
          continue;
        }
        box_markers.push_back(*it); // No markers in res overlaps. No need to to search and merge overlaps
        it = res.erase(it);
      }

      for (auto it = ads.begin(); it != ads.end();) {
        if (!is_in_rect(morton::decode(it->morton_code), box_min, box_max)) {
          ++it;
          continue;
        }
        merge_marker(box_markers, {pointf_from_morton(it->morton_code), it->count, true}, box_side / 2);
        it = ads.erase(it);
      }

      for (auto it = poi.begin(); it != poi.end();) {
        if (!is_in_rect(morton::decode(it->morton_code), box_min, box_max)) {
          ++it;
          continue;
        }
        merge_marker(box_markers, {pointf_from_morton(it->morton_code), it->count, false}, box_side / 2);
        it = poi.erase(it);
      }
      std::move(box_markers.begin(), box_markers.end(), std::back_inserter(res));
      box_markers.clear();
    }
  }
  return res;
}

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
                       try {
                         poi_data data = res.futures[res.index].get(); // TODO: handle network errors here
                         if (res.index == 1 && data.regular.empty() && data.advertized.empty())
                           return std::move(res.futures[0]);
                         return pc::make_ready_future(std::move(data));
                       } catch (network_error err) { // TODO: network error
                         assert(res.index == 0);
                         qWarning("POI download load failed: %s", err.what());
                         return std::move(res.futures[1]);
                       }
                     })
                     .then(notify);
}

pc::future<std::vector<marker>> poidb::generalize(const QRectF& viewport, int z_level) const {
  const uint64_t min = pointf_to_morton(viewport.topLeft());
  const uint64_t max = pointf_to_morton(viewport.bottomRight());

  auto generalize_func = [min, max, z_level](std::shared_ptr<const std::vector<uint64_t>> points) {
    return ::generalize(*points, min, max, z_level);
  };
  std::array<pc::future<std::vector<point_group>>, 2> futures = {
      pc::async(QThreadPool::globalInstance(), generalize_func,
          std::shared_ptr<const std::vector<uint64_t>>{data_, &data_->advertized}),
      pc::async(QThreadPool::globalInstance(), generalize_func,
          std::shared_ptr<const std::vector<uint64_t>>{data_, &data_->regular})};
  return pc::when_all(std::make_move_iterator(futures.begin()), std::make_move_iterator(futures.end()))
      .next([min, max, z_level](std::vector<pc::future<std::vector<point_group>>> results) {
        return merge_generalizations(results[0].get(), results[1].get(), min, max, z_level);
      });
}

void poidb::on_loaded() {
  if (!load_future_.valid() || !load_future_.is_ready())
    return;
  data_ = std::make_shared<poi_data>(load_future_.get());
  emit updated();
}
