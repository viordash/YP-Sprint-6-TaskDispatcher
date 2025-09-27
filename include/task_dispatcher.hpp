#pragma once

#include "queue/priority_queue.hpp"
#include "thread_pool/thread_pool.hpp"
#include "types.hpp"
#include <map>

namespace dispatcher {

class TaskDispatcher {
protected:
    std::shared_ptr<queue::PriorityQueue> priority_queue;
    std::unique_ptr<thread_pool::ThreadPool> thread_pool;

    static constexpr std::map<TaskPriority, queue::QueueOptions> get_default_queue_options() {
        return {
            {TaskPriority::High, {true, 1000}},  //
            {TaskPriority::Normal, {false, {}}}  //
        };
    }

public:
    explicit TaskDispatcher(size_t thread_count, const std::map<TaskPriority, queue::QueueOptions> &queue_options =
                                                     get_default_queue_options());

    void schedule(TaskPriority priority, std::function<void()> task);
    ~TaskDispatcher();
};

}  // namespace dispatcher