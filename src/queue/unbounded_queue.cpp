#include "queue/unbounded_queue.hpp"

namespace dispatcher::queue {

UnboundedQueue::UnboundedQueue(int capacity) { (void)capacity; }

void UnboundedQueue::push(std::function<void()> task) {
    std::lock_guard<std::mutex> lock(mutex);
    queue.push(std::move(task));
}

std::optional<std::function<void()>> UnboundedQueue::try_pop() {
    std::lock_guard<std::mutex> lock(mutex);
    if (queue.empty()) {
        return std::nullopt;
    }
    auto task = std::move(queue.front());
    queue.pop();
    return task;
}

UnboundedQueue::~UnboundedQueue() = default;

}  // namespace dispatcher::queue