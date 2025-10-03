#include "queue/priority_queue.hpp"
#include <atomic>
#include <cassert>

namespace dispatcher::queue {

PriorityQueue::PriorityQueue(const std::unordered_map<TaskPriority, QueueOptions> &options) {
    for (auto const &[priority, opts] : options) {
        if (opts.bounded) {
            assert(opts.capacity.has_value() && opts.capacity.value() > 0);
            queues[priority] = std::make_unique<BoundedQueue>(opts.capacity.value());
        } else {
            queues[priority] = std::make_unique<UnboundedQueue>();
        }
    }
    bool least_one_queue_must_be = !queues.empty();
    assert(least_one_queue_must_be);
}

void PriorityQueue::push(TaskPriority priority, std::function<void()> task) {
    std::unique_lock<std::mutex> lock(mutex);
    auto it = queues.find(priority);
    bool priority_not_orphaned = it != queues.end();
    assert(priority_not_orphaned);

    it->second->push(std::move(task));
    lock.unlock();
    not_empty.notify_one();
}

std::optional<std::function<void()>> PriorityQueue::pop() {
    std::unique_lock<std::mutex> lock(mutex);

    while (true) {
        for (const auto &q : queues) {
            auto task = q.second->try_pop();
            if (task.has_value()) {
                return task;
            }
        }

        if (shutdown_flag.load(std::memory_order_acquire)) {
            break;
        }
        not_empty.wait(lock);
    }
    return std::nullopt;
}

void PriorityQueue::shutdown() {
    shutdown_flag.store(true, std::memory_order_release);
    not_empty.notify_all();
}

PriorityQueue::~PriorityQueue() { shutdown(); }

}  // namespace dispatcher::queue