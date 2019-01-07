#include <QtCore/QUrl>

#include <QtGui/QImage>

#include <QtNetwork/QNetworkReply>

#include <portable_concurrency/future>

#include <mapex/tile_loader.hpp>
#include <mapex/promised_reply.hpp>

namespace {

QUrl get_tile_url(int x, int y, int z_level) {
  return QUrl{
    QStringLiteral("https://tile1.maps.2gis.com/tiles?x=%1&y=%2&z=%3&v=1.5&r=g&ts=online_sd")
      .arg(x)
      .arg(y)
      .arg(z_level)
  };
}

} // namespace

pc::future<QImage> load_tile(QNetworkAccessManager& nm, int x, int y, int z_level) {
  auto* reply = new promised_reply{nm.get(QNetworkRequest{get_tile_url(x, y, z_level)}), &nm};
  return reply->get_future().next([](QNetworkReply* reply) {
    QImage res;
    res.load(reply, "PNG"); // TODO extract format from reply MIME type
    return res;
  });
}
