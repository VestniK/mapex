#include <cassert>

#include <QtCore/QPointer>

#include <QtNetwork/QNetworkReply>

#include <portable_concurrency/future>

#include <mapex/network_thread.hpp>
#include <mapex/qnetwork_category.hpp>

namespace {

// Self-deletes on reply finished. Deletes reply on error.
class promised_reply final : public QObject {
  Q_OBJECT
public:
  promised_reply(QNetworkReply* reply, QObject* parent = nullptr)
      : QObject{parent}, promise_{pc::canceler_arg, [reply = QPointer<QNetworkReply>{reply}, nm = reply->manager()] {
                                    post(nm, [reply] {
                                      if (!reply.isNull())
                                        reply->abort();
                                    });
                                  }} {
    reply->setParent(this);
    reply->setObjectName("reply");
    QMetaObject::connectSlotsByName(this);
  }

  [[nodiscard]] pc::future<std::unique_ptr<QNetworkReply>> get_future() { return promise_.get_future(); }

private slots:
  void on_reply_finished() {
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

pc::future<std::unique_ptr<QNetworkReply>> send_request(QNetworkAccessManager& nm, const QUrl& url) {
  auto* reply = new promised_reply{nm.get(QNetworkRequest{url}), &nm};
  return reply->get_future();
}

network_thread::network_thread() {
  QObject::connect(&thread_, &QThread::finished, &nm_, [this] { nm_.moveToThread(nullptr); }, Qt::DirectConnection);
  nm_.moveToThread(&thread_);
  thread_.setObjectName("mapex_network");
  thread_.start();
}

network_thread::~network_thread() {
  if (!thread_.isFinished())
    shotdown();
}

void network_thread::shotdown() {
  thread_.quit();
  thread_.wait();
}

#include "network_thread.moc"
