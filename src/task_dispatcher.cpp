#include "task_dispatcher.hpp"
#include "logger.hpp"
#include <format>

namespace dispatcher {

TaskDispatcher::TaskDispatcher(size_t thread_count, const std::map<TaskPriority, queue::QueueOptions> &queue_options)
    : priority_queue(std::make_shared<queue::PriorityQueue>(queue_options)),
      thread_pool(std::make_unique<thread_pool::ThreadPool>(priority_queue, thread_count)) {

    Logger::Get().Log(std::format("TaskDispatcher initialized with {} threads.", thread_count));
}

void TaskDispatcher::schedule(TaskPriority priority, std::function<void()> task) {
    priority_queue->push(priority, std::move(task));
}

TaskDispatcher::~TaskDispatcher() = default;

}  // namespace dispatcher