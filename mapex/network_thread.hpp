#pragma once

#include <QtCore/QThread>
#include <QtNetwork/QNetworkAccessManager>

#include <mapex/executors.hpp>

class network_thread {
public:
  using executor_type = QObject*;

  network_thread();
  ~network_thread();

  /// Must never be used inside a task posted to `this->executor()`
  void shotdown();

  /// @threadsafe
  executor_type executor() noexcept { return &nm_; }

  /// Must only be used inside a task posted to `this->executor()`
  QNetworkAccessManager& network_manager() noexcept { return nm_; }

private:
  QThread thread_;
  QNetworkAccessManager nm_;
};
