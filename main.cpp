#include <cmath>

#include <QtCore/QUrl>

#include <QtWidgets/QApplication>
#include <QtWidgets/QGraphicsView>

namespace {

QUrl get_tile_url(int x, int y, int z_level) {
  return QUrl{
    QStringLiteral("https://tile1.maps.2gis.com/tiles?x=%1&y=%2&z=%3&v=1.5&r=g&ts=online_sd")
      .arg(x)
      .arg(y)
      .arg(z_level)
  };
}

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

enum class geo_coord {longiture, lattitude};
using longitude = unit<double, geo_coord::longiture>;
constexpr longitude operator"" _lon(long double val) noexcept {return longitude{static_cast<double>(val)};}
using lattitude = unit<double, geo_coord::lattitude>;
constexpr lattitude operator"" _lat(long double val) noexcept {return lattitude{static_cast<double>(val)};}

struct geo_point {
  longitude lon;
  lattitude lat;
};

constexpr geo_point nsk_center = {82.947932_lon, 54.988053_lat};

struct tile_coord {
  int x = 0;
  int y = 0;
};

struct projected_point {
  tile_coord tile;
  QPoint point;
};

constexpr double deg2rad(double deg) noexcept {
  return deg*M_PI/180.;
}

projected_point project(geo_point point, int z_level) noexcept {
  const double pow2z = (2 << z_level);
  double tile_x;
  const double x = std::modf((static_cast<double>(point.lon) + 180.)/360.*pow2z, &tile_x);

  const double lat_rad = deg2rad(static_cast<double>(point.lat));
  double tile_y;
  const double y = std::modf((1. - std::log(std::tan(lat_rad) + 1./std::cos(lat_rad))/M_PI)/2.*pow2z, &tile_y);

  constexpr int tile_pixel_size = 256;
  return {
    {static_cast<int>(tile_x), static_cast<int>(tile_y)},
    {static_cast<int>(std::floor(x*tile_pixel_size)), static_cast<int>(std::floor(y*tile_pixel_size))}
  };
}

} // namespace

int main(int argc, char** argv) {
  QApplication app{argc, argv};
  QGraphicsView wnd;
  wnd.show();
  return app.exec();
}
