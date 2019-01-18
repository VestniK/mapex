#include <QtCore/QMetaObject>
#include <QtCore/QObject>
#include <QtCore/QThreadPool>

#include <mapex/executors.hpp>

namespace {

class task_runable: public QRunnable {
public:
  task_runable(pc::unique_function<void()> task): task_{std::move(task)} {}

  void run() override {task_();}

private:
  pc::unique_function<void()> task_;
};

} // namespace

void post(QObject* obj, pc::unique_function<void()> task) {
  QMetaObject::invokeMethod(
    obj,
    [task = std::make_shared<pc::unique_function<void()>>(std::move(task))] {(*task)();},
    Qt::QueuedConnection
  );
}

void post(QThreadPool* pool, pc::unique_function<void()> task) {
 pool->start(new task_runable{std::move(task)});
}
