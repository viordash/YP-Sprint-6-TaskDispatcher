#include "thread_pool/thread_pool.hpp"
#include <atomic>
#include <gtest/gtest.h>
#include <print>
#include <thread>
#include <vector>

namespace dispatcher::thread_pool {

using namespace dispatcher::queue;
using namespace std::chrono_literals;

class TestableThreadPool : ThreadPool {
public:
    static std::shared_ptr<PriorityQueue> create_priority_queue() {
        std::map<TaskPriority, QueueOptions> options;
        options[TaskPriority::High] = {false, std::nullopt};
        options[TaskPriority::Normal] = {false, std::nullopt};
        return std::make_shared<PriorityQueue>(options);
    }

    TestableThreadPool(size_t thread_count) : ThreadPool(create_priority_queue(), thread_count) {}
    const std::vector<std::jthread> &PublicMorozov_workers() { return workers; }
    const std::shared_ptr<queue::PriorityQueue> PublicMorozov_priority_queue() { return priority_queue; }
};

TEST(ThreadPoolTest, Constructor_creates_workers) {
    TestableThreadPool pool(3);

    ASSERT_EQ(pool.PublicMorozov_workers().size(), 3);
}

TEST(ThreadPoolTest, Executes_tasks_from_priority_queue) {
    TestableThreadPool pool(2);

    std::atomic<int> counter = 0;
    const int num_tasks = 100;
    auto pq = pool.PublicMorozov_priority_queue();

    for (int i = 0; i < num_tasks; ++i) {
        pq->push(TaskPriority::Normal, [&]() { counter++; });
    }

    while (counter.load() < num_tasks) {
        std::this_thread::sleep_for(10ms);
    }
    ASSERT_EQ(counter.load(), num_tasks);
}

TEST(ThreadPoolTest, Destructor_shutsdown_workers_gracefully) {
    std::atomic<int> tasks_executed = 0;
    const int num_tasks = 20;

    auto pq = TestableThreadPool::create_priority_queue();
    {
        ThreadPool pool_with_short_life(pq, 2);

        for (int i = 0; i < num_tasks; ++i) {
            pq->push(TaskPriority::Normal, [&]() {
                tasks_executed++;
                std::this_thread::sleep_for(10ms);
            });
        }
    }
    ASSERT_EQ(tasks_executed.load(), num_tasks);
    bool queue_is_empty = !pq->pop().has_value();
    ASSERT_TRUE(queue_is_empty);
}

TEST(ThreadPoolTest, Handles_no_tasks_during_Shutdown) {
    auto pq = TestableThreadPool::create_priority_queue();

    {
        ThreadPool pool_without_tasks(pq, 1);
    }

    ASSERT_FALSE(pq->pop().has_value());
}

}  // namespace dispatcher::thread_pool