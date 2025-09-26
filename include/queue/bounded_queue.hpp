#pragma once
#include "queue/queue.hpp"

#include <condition_variable>
#include <functional>
#include <mutex>
#include <optional>
#include <queue>

namespace dispatcher::queue {

class BoundedQueue : public IQueue {
private:
    std::queue<std::function<void()>> queue;
    std::mutex mutex;
    std::condition_variable not_full;
    int capacity;

public:
    explicit BoundedQueue(int capacity);

    void push(std::function<void()> task) override;

    std::optional<std::function<void()>> try_pop() override;

    ~BoundedQueue() override;
};

}  // namespace dispatcher::queue