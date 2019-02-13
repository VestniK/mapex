#pragma once

#include <memory>

#include <QtCore/QThread>
#include <QtNetwork/QNetworkAccessManager>

#include <portable_concurrency/future_fwd>

#include <mapex/executors.hpp>

class QNetworkReply;
class QUrl;

class network_error : public std::system_error {
public:
  network_error(std::error_code ec, const std::string& message) : std::system_error{ec, message} {}
  network_error(int cond, const std::error_category& category, const std::string& message)
      : std::system_error{cond, category, message} {}
};

class network_thread {
public:
  network_thread();
  ~network_thread();

  /// Must never be used inside a task posted to `this->executor()`
  void shotdown();

  /// @threadsafe
  [[nodiscard]] pc::future<std::unique_ptr<QNetworkReply>> send_request(const QUrl& url);

private:
  QThread thread_;
  QNetworkAccessManager nm_;
};
