#include <QtCore/QMetaObject>
#include <QtNetwork/QNetworkReply>

#include <mapex/promised_reply.hpp>
#include <mapex/qnetwork_category.hpp>

promised_reply::promised_reply(QNetworkReply* reply, QObject* parent) : QObject{parent} {
  reply->setParent(this);
  reply->setObjectName("reply");
  QMetaObject::connectSlotsByName(this);
}

void promised_reply::on_reply_finished() {
  deleteLater();
  if (std::exchange(promise_satisfied_, true))
    return;
  auto* reply = qobject_cast<QNetworkReply*>(sender());
  reply->setParent(nullptr);
  promise_.set_value(std::unique_ptr<QNetworkReply>{reply});
}

void promised_reply::on_reply_error(QNetworkReply::NetworkError err) {
  deleteLater();
  if (std::exchange(promise_satisfied_, true))
    return;
  auto* reply = qobject_cast<QNetworkReply*>(sender());
  promise_.set_exception(std::make_exception_ptr(std::system_error{err, reply->errorString().toStdString()}));
}

void promised_reply::on_reply_sslErrors(const QList<QSslError>&) {
  deleteLater();
  if (std::exchange(promise_satisfied_, true))
    return;
  auto* reply = qobject_cast<QNetworkReply*>(sender());
  promise_.set_exception(std::make_exception_ptr(std::runtime_error{"SSL Error"})); // TODO std::system_error
}
