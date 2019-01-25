#include <QtWidgets/QApplication>

#include <mapex/geo_point.hpp>
#include <mapex/network_thread.hpp>
#include <mapex/tile_widget.hpp>

#include "ui_controls.h"

namespace {

constexpr geo_point nsk_center = {82.947932_lon, 54.988053_lat};

} // namespace

int main(int argc, char** argv) {
  QApplication app{argc, argv};

  network_thread net_thread;
  tile_widget wnd{nsk_center, 12, &net_thread};
  Ui::map_contorls controls;
  controls.setupUi(&wnd);
  QObject::connect(controls.showPOI, &QPushButton::toggled, &wnd, &tile_widget::set_poi_visible);

  wnd.show();
  return app.exec();
}
