#pragma once
#include "queue/bounded_queue.hpp"
#include "queue/unbounded_queue.hpp"
#include "types.hpp"
#include <atomic>
#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>

namespace dispatcher::queue {

class PriorityQueue {
protected:
    std::map<TaskPriority, std::unique_ptr<IQueue>> queues;
    std::mutex mutex;
    std::condition_variable not_empty;
    bool shutdown_flag = false;

public:
    explicit PriorityQueue(const std::unordered_map<TaskPriority, QueueOptions> &options);

    void push(TaskPriority priority, std::function<void()> task);
    // block on pop until shutdown is called
    // after that return std::nullopt on empty queue
    std::optional<std::function<void()>> pop();

    void shutdown();

    ~PriorityQueue();
};

}  // namespace dispatcher::queue