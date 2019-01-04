#include <QtCore/QUrl>

#include <QtWidgets/QApplication>
#include <mapex/tile_widget.hpp>

namespace {

constexpr geo_point nsk_center = {82.947932_lon, 54.988053_lat};

} // namespace

int main(int argc, char** argv) {
  QApplication app{argc, argv};
  tile_widget wnd{nsk_center, 12};
  wnd.show();
  return app.exec();
}
