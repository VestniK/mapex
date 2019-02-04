#include <QtCore/QThreadPool>
#include <QtCore/QUrl>

#include <QtGui/QImage>
#include <QtGui/QImageReader>

#include <QtNetwork/QNetworkReply>

#include <portable_concurrency/future>

#include <mapex/executors.hpp>
#include <mapex/network_thread.hpp>
#include <mapex/tile_loader.hpp>

namespace {

QUrl get_tile_url(int x, int y, int z_level) {
  return QUrl{QStringLiteral("https://tile1.maps.2gis.com/tiles?x=%1&y=%2&z=%3&v=1.5&r=g&ts=online_sd")
                  .arg(x)
                  .arg(y)
                  .arg(z_level)};
}

} // namespace

pc::future<QImage> load_tile(QNetworkAccessManager& nm, int x, int y, int z_level) {
  return send_request(nm, get_tile_url(x, y, z_level))
      .next(QThreadPool::globalInstance(), [](std::unique_ptr<QNetworkReply> reply) {
        const auto mime = reply->header(QNetworkRequest::ContentTypeHeader).toByteArray();
        if (!QImageReader::supportedMimeTypes().contains(mime))
          throw std::runtime_error{"unsupported image MIME type + " + mime.toStdString()};

        const auto formats = QImageReader::imageFormatsForMimeType(mime);
        if (formats.empty())
          throw std::runtime_error{"no known formats for MIME + " + mime.toStdString()};

        QImageReader reader{reply.get(), formats.first()};
        QImage res;
        if (!reader.read(&res))
          throw std::runtime_error{"failed to load image: " + reader.errorString().toStdString()};
        return res;
      });
}
