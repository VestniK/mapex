#include <cassert>

#include <mapex/network_thread.hpp>

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
