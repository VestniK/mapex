#include <QtNetwork/QNetworkReply>
#include <QtCore/QMetaObject>

#include <mapex/promised_reply.hpp>

promised_reply::promised_reply(QNetworkReply* reply, QObject* parent):
  QObject{parent}
{
  reply->setParent(this);
  reply->setObjectName("reply");
  QMetaObject::connectSlotsByName(this);
}


void promised_reply::on_reply_finished() {
  deleteLater();
  auto* reply = qobject_cast<QNetworkReply*>(sender());
  disconnect(this, nullptr, reply, nullptr);
  promise_.set_value(reply);
}

void promised_reply::on_reply_error(QNetworkReply::NetworkError) {
  deleteLater();
  auto* reply = qobject_cast<QNetworkReply*>(sender());
  disconnect(this, nullptr, reply, nullptr);
  promise_.set_exception(std::make_exception_ptr(std::runtime_error{"Error"})); // TODO std::system_error
}

void promised_reply::on_reply_sslErrors(const QList<QSslError>&) {
  deleteLater();
  auto* reply = qobject_cast<QNetworkReply*>(sender());
  disconnect(this, nullptr, reply, nullptr);
  promise_.set_exception(std::make_exception_ptr(std::runtime_error{"SSL Error"}));}  // TODO std::system_error
