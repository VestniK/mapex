#include <cassert>

#include <mapex/network_thread.hpp>

network_thread* network_thread::ms_instance = nullptr;

network_thread::network_thread(instance_scope scope) {
  QObject::connect(&thread_, &QThread::finished, &nm_, [this] {nm_.moveToThread(nullptr);}, Qt::DirectConnection);
  nm_.moveToThread(&thread_);
  thread_.setObjectName("mapex_network");
  thread_.start();
  if (scope == instance_scope::local)
    return;
  assert(!ms_instance);
  ms_instance = this;
}

network_thread::~network_thread() {
  if (ms_instance == this)
    ms_instance = nullptr;
  if (!thread_.isFinished())
    shotdown();
}

void network_thread::shotdown() {
  thread_.quit();
  thread_.wait();
}
