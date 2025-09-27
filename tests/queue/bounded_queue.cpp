#include "queue/bounded_queue.hpp"
#include <atomic>
#include <gtest/gtest.h>
#include <print>
#include <thread>

namespace dispatcher::queue {

TEST(BoundedQueueTest, Push_and_TryPop_single_thread) {
    BoundedQueue bounded_queue(1);
    bool task_executed = false;

    bounded_queue.push([&]() { task_executed = true; });
    auto task = bounded_queue.try_pop();

    ASSERT_TRUE(task.has_value());
    task.value()();
    ASSERT_TRUE(task_executed);

    auto empty_task = bounded_queue.try_pop();
    ASSERT_FALSE(empty_task.has_value());
}

TEST(BoundedQueueTest, TryPop_on_empty_queue) {
    BoundedQueue bounded_queue(2);
    auto task = bounded_queue.try_pop();
    ASSERT_FALSE(task.has_value());
}

TEST(BoundedQueueTest, Queue_has_exceeded_capacity) {
    BoundedQueue bounded_queue(2);
    int counter = 0;

    bounded_queue.push([&]() { counter++; });
    bounded_queue.push([&]() { counter++; });

    std::atomic<bool> push_blocked = false;
    std::jthread thread_with_push_blocked([&]() {
        push_blocked = true;
        push_blocked.notify_all();
        bounded_queue.push([&]() { counter += 40; });
        push_blocked = false;
        push_blocked.notify_all();
    });

    push_blocked.wait(false);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ASSERT_TRUE(push_blocked);

    auto task1 = bounded_queue.try_pop();
    ASSERT_TRUE(task1.has_value());
    task1.value()();

    push_blocked.wait(true);
    ASSERT_FALSE(push_blocked);

    auto task2 = bounded_queue.try_pop();
    ASSERT_TRUE(task2.has_value());
    task2.value()();

    auto task3 = bounded_queue.try_pop();
    ASSERT_TRUE(task3.has_value());
    task3.value()();

    ASSERT_EQ(counter, 42);
}

TEST(BoundedQueueTest, Push_and_TryPop_multi_thread) {
    const int capacity = 5;
    const int num_tasks = 100;
    BoundedQueue bounded_queue(capacity);
    std::atomic<int> counter = 0;

    auto push_func = [&]() {
        for (int i = 0; i < num_tasks; ++i) {
            bounded_queue.push([&]() { counter++; });
        }
    };

    auto pop_func = [&]() {
        for (size_t i = 0; i < num_tasks; i++) {
            std::optional<std::function<void()>> task;
            while (!(task = bounded_queue.try_pop()).has_value()) {
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

    ASSERT_EQ(counter, num_tasks * 3);
}

}  // namespace dispatcher::queue