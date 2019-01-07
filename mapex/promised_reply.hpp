#pragma once

#include <QtCore/QObject>

#include <QtNetwork/QNetworkReply>

#include <portable_concurrency/future>

class promised_reply final: public QObject {
  Q_OBJECT
public:
  promised_reply(QNetworkReply* reply, QObject* parent = nullptr);

  [[nodiscard]]
  pc::future<QNetworkReply*> get_future() {return promise_.get_future();}

private slots:
  void on_reply_finished();
  void on_reply_error(QNetworkReply::NetworkError);
  void on_reply_sslErrors(const QList<QSslError> &errors);

private:
  pc::promise<QNetworkReply*> promise_;
};
