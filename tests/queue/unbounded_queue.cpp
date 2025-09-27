#include "queue/unbounded_queue.hpp"
#include <atomic>
#include <gtest/gtest.h>
#include <thread>

namespace dispatcher::queue {

TEST(UnboundedQueueTest, Push_and_TryPop_single_thread) {
    UnboundedQueue unbounded_queue;
    bool task_executed = false;

    unbounded_queue.push([&]() { task_executed = true; });
    auto task = unbounded_queue.try_pop();

    ASSERT_TRUE(task.has_value());
    task.value()();
    ASSERT_TRUE(task_executed);

    auto empty_task = unbounded_queue.try_pop();
    ASSERT_FALSE(empty_task.has_value());
}

TEST(UnboundedQueueTest, TryPop_on_empty_queue) {
    UnboundedQueue unbounded_queue;
    auto task = unbounded_queue.try_pop();
    ASSERT_FALSE(task.has_value());
}

TEST(UnboundedQueueTest, Multiple_Pushes_and_Pops_single_thread) {
    UnboundedQueue unbounded_queue;
    int counter = 0;
    const int num_items = 10;

    for (int i = 0; i < num_items; ++i) {
        unbounded_queue.push([&]() { counter++; });
    }

    for (int i = 0; i < num_items; ++i) {
        auto task = unbounded_queue.try_pop();
        ASSERT_TRUE(task.has_value());
        task.value()();
    }
    EXPECT_EQ(counter, num_items);

    auto empty_task = unbounded_queue.try_pop();
    ASSERT_FALSE(empty_task.has_value());
}

TEST(UnboundedQueueTest, Push_and_TryPop_multi_thread) {
    const int num_tasks = 1000;
    UnboundedQueue unbounded_queue;
    std::atomic<int> counter = 0;

    auto push_func = [&]() {
        for (int i = 0; i < num_tasks; ++i) {
            unbounded_queue.push([&]() { counter++; });
        }
    };

    auto pop_func = [&]() {
        for (size_t i = 0; i < num_tasks; i++) {
            std::optional<std::function<void()>> task;
            while (!(task = unbounded_queue.try_pop()).has_value()) {
                std::this_thread::yield();
            }
            task.value()();
        }
    };

    {
        std::jthread push_thread1(push_func);
        std::jthread pop_thread1(pop_func);
        std::jthread push_thread2(push_func);
        std::jthread pop_thread2(pop_func);
        std::jthread push_thread3(push_func);
        std::jthread pop_thread3(pop_func);
    }

    EXPECT_EQ(counter, num_tasks * 3);
}

}  // namespace dispatcher::queue