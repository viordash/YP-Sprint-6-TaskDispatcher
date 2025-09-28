#pragma once

#include "queue/priority_queue.hpp"
#include <memory>
#include <thread>
#include <vector>

namespace dispatcher::thread_pool {

class ThreadPool {
protected:
    std::shared_ptr<queue::PriorityQueue> priority_queue;
    std::vector<std::jthread> workers;

    void worker();

public:
    explicit ThreadPool(std::shared_ptr<queue::PriorityQueue> priority_queue, size_t thread_count);
    ~ThreadPool();
};

}  // namespace dispatcher::thread_pool
