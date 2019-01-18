#pragma once

#include <QtCore/QThread>
#include <QtNetwork/QNetworkAccessManager>

#include <mapex/executors.hpp>

enum class instance_scope { local, global };

class network_thread {
public:
  using executor_type = QObject*;

  explicit network_thread(instance_scope scope = instance_scope::local);

  ~network_thread();

  /// Must never be used inside a task posted to `this->executor()`
  void shotdown();

  /// @threadsafe
  static network_thread* instance() noexcept { return ms_instance; }

  /// @threadsafe
  executor_type executor() noexcept { return &nm_; }

  /// Must only be used inside a task posted to `this->executor()`
  QNetworkAccessManager& network_manager() noexcept { return nm_; }

private:
  static network_thread* ms_instance;

private:
  QThread thread_;
  QNetworkAccessManager nm_;
};
