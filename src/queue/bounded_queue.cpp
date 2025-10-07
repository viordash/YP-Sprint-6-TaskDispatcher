#include "queue/bounded_queue.hpp"
#include <cassert>

namespace dispatcher::queue {

BoundedQueue::BoundedQueue(int capacity) : capacity{capacity} { assert(capacity > 0); }

void BoundedQueue::push(std::function<void()> task) {
    std::unique_lock<std::mutex> lock(mutex);
    not_full.wait(lock, [this] { return queue.size() < capacity; });
    queue.push(std::move(task));
}

std::optional<std::function<void()>> BoundedQueue::try_pop() {
    std::unique_lock<std::mutex> lock(mutex);
    if (queue.empty()) {
        return std::nullopt;
    }
    auto task = std::move(queue.front());
    queue.pop();

    lock.unlock();
    not_full.notify_one();
    return task;
}

BoundedQueue::~BoundedQueue() = default;

}  // namespace dispatcher::queue