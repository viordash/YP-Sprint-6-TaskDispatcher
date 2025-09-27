#include "thread_pool/thread_pool.hpp"
#include "logger.hpp"
#include <exception>

namespace dispatcher::thread_pool {

ThreadPool::ThreadPool(std::shared_ptr<queue::PriorityQueue> priority_queue, size_t thread_count)
    : priority_queue(std::move(priority_queue)) {
    workers.reserve(thread_count);
    for (size_t i = 0; i < thread_count; ++i) {
        workers.emplace_back(&ThreadPool::worker, this);
    }
}

void ThreadPool::worker() {
    Logger::Get().Log("Worker runned.");
    while (true) {
        auto task = priority_queue->pop();
        if (!task.has_value()) {
            Logger::Get().Log("Shutdown Worker");
            break;
        }
        try {
            task.value()();
        } catch (const std::exception &ex) {
            Logger::Get().Log(std::format("Task error: {}", ex.what()));
        } catch (...) {
            Logger::Get().Log("Task unknown error");
        }
    }
}

ThreadPool::~ThreadPool() { priority_queue->shutdown(); }

}  // namespace dispatcher::thread_pool