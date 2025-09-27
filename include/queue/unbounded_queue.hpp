#pragma once
#include "queue/queue.hpp"
#include <functional>
#include <mutex>
#include <optional>
#include <queue>

namespace dispatcher::queue {

class UnboundedQueue : public IQueue {
private:
    std::queue<std::function<void()>> queue;
    std::mutex mutex;

public:
    explicit UnboundedQueue(int capacity = 0);

    void push(std::function<void()> task) override;

    std::optional<std::function<void()>> try_pop() override;

    ~UnboundedQueue() override;
};

}  // namespace dispatcher::queue