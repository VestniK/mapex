#pragma once

#include <portable_concurrency/execution>
#include <portable_concurrency/functional>

class QObject;
class QThreadPool;

namespace portable_concurrency {

template <>
struct is_executor<QObject*> : std::true_type {};
template <>
struct is_executor<QThreadPool*> : std::true_type {};

} // namespace portable_concurrency

void post(QObject* obj, pc::unique_function<void()> task);
void post(QThreadPool* pool, pc::unique_function<void()> task);
