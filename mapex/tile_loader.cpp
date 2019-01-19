#include <QtCore/QThreadPool>
#include <QtCore/QUrl>

#include <QtGui/QImage>
#include <QtGui/QImageReader>

#include <QtNetwork/QNetworkReply>

#include <portable_concurrency/future>

#include <mapex/executors.hpp>
#include <mapex/qnetwork_category.hpp>
#include <mapex/tile_loader.hpp>

namespace {

QUrl get_tile_url(int x, int y, int z_level) {
  return QUrl{QStringLiteral("https://tile1.maps.2gis.com/tiles?x=%1&y=%2&z=%3&v=1.5&r=g&ts=online_sd")
                  .arg(x)
                  .arg(y)
                  .arg(z_level)};
}

// Self-deletes on reply finished. Deletes reply on error.
class promised_reply final : public QObject {
  Q_OBJECT
public:
  promised_reply(QNetworkReply* reply, QObject* parent = nullptr) : QObject{parent} {
    reply->setParent(this);
    reply->setObjectName("reply");
    QMetaObject::connectSlotsByName(this);
  }

  [[nodiscard]] pc::future<std::unique_ptr<QNetworkReply>> get_future() { return promise_.get_future(); }

  private slots : void on_reply_finished() {
    deleteLater();
    if (std::exchange(promise_satisfied_, true))
      return;
    auto* reply = qobject_cast<QNetworkReply*>(sender());
    reply->setParent(nullptr);
    promise_.set_value(std::unique_ptr<QNetworkReply>{reply});
  }

  void on_reply_error(QNetworkReply::NetworkError err) {
    deleteLater();
    if (std::exchange(promise_satisfied_, true))
      return;
    auto* reply = qobject_cast<QNetworkReply*>(sender());
    promise_.set_exception(std::make_exception_ptr(std::system_error{err, reply->errorString().toStdString()}));
  }

  void on_reply_sslErrors(const QList<QSslError>&) {
    deleteLater();
    if (std::exchange(promise_satisfied_, true))
      return;
    auto* reply = qobject_cast<QNetworkReply*>(sender());
    promise_.set_exception(std::make_exception_ptr(std::runtime_error{"SSL Error"})); // TODO std::system_error
  }

private:
  pc::promise<std::unique_ptr<QNetworkReply>> promise_;
  bool promise_satisfied_ = false;
};

} // namespace

pc::future<QImage> load_tile(QNetworkAccessManager& nm, int x, int y, int z_level) {
  auto* reply = new promised_reply{nm.get(QNetworkRequest{get_tile_url(x, y, z_level)}), &nm};
  return reply->get_future().next(QThreadPool::globalInstance(), [](std::unique_ptr<QNetworkReply> reply) {
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

#include "tile_loader.moc"
