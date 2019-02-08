#include <QtCore/QDir>
#include <QtCore/QThreadPool>

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
  QDir::addSearchPath("icons", ":/icons");

  network_thread net_thread;
  tile_widget wnd{nsk_center, 12, &net_thread};
  Ui::map_contorls controls;
  controls.setupUi(&wnd);
  QObject::connect(controls.showPOI, &QPushButton::toggled, &wnd, &tile_widget::set_poi_visible);
  QObject::connect(&app, &QCoreApplication::aboutToQuit, [&net_thread] {
    net_thread.shotdown();
    QThreadPool::globalInstance()->clear();
    QThreadPool::globalInstance()->waitForDone();
  });

  wnd.show();
  return app.exec();
}
