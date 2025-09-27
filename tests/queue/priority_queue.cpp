#include "queue/priority_queue.hpp"
#include <atomic>
#include <chrono>
#include <gtest/gtest.h>
#include <iterator>
#include <string>
#include <thread>
#include <vector>

namespace dispatcher::queue {

using namespace std::chrono_literals;

class TestablePriorityQueue : PriorityQueue {
public:
    TestablePriorityQueue(const std::map<TaskPriority, QueueOptions> &options) : PriorityQueue(options) {}
    const std::map<TaskPriority, std::unique_ptr<IQueue>> &PublicMorozov_queues() { return queues; }
};

TEST(PriorityQueueTest, Constructor_creates_queues) {
    std::map<TaskPriority, QueueOptions> options;
    options[TaskPriority::High] = {true, 10};
    options[TaskPriority::Normal] = {false, std::nullopt};

    TestablePriorityQueue priority_queue(options);

    ASSERT_EQ((*priority_queue.PublicMorozov_queues().begin()).first, TaskPriority::High);
    ASSERT_EQ((*std::next(priority_queue.PublicMorozov_queues().begin())).first, TaskPriority::Normal);
}

TEST(PriorityQueueTest, PushAndPop_single_thread_respects_priority) {
    std::map<TaskPriority, QueueOptions> options;
    options[TaskPriority::High] = {false, std::nullopt};
    options[TaskPriority::Normal] = {false, std::nullopt};
    PriorityQueue priority_queue(options);

    std::vector<std::string> results;
    std::mutex results_mutex;
    priority_queue.push(TaskPriority::Normal, [&] {
        std::lock_guard<std::mutex> lock(results_mutex);
        results.push_back("Normal 1");
    });
    priority_queue.push(TaskPriority::High, [&] {
        std::lock_guard<std::mutex> lock(results_mutex);
        results.push_back("High 2");
    });
    priority_queue.push(TaskPriority::Normal, [&] {
        std::lock_guard<std::mutex> lock(results_mutex);
        results.push_back("Normal 3");
    });
    priority_queue.push(TaskPriority::High, [&] {
        std::lock_guard<std::mutex> lock(results_mutex);
        results.push_back("High 4");
    });

    auto taskHigh_2 = priority_queue.pop();
    auto taskHigh_4 = priority_queue.pop();
    auto taskNormal_1 = priority_queue.pop();
    auto taskNormal_3 = priority_queue.pop();

    ASSERT_TRUE(taskHigh_2.has_value());
    ASSERT_TRUE(taskHigh_4.has_value());
    ASSERT_TRUE(taskNormal_1.has_value());
    ASSERT_TRUE(taskNormal_3.has_value());

    taskHigh_2.value()();
    taskHigh_4.value()();
    taskNormal_1.value()();
    taskNormal_3.value()();

    ASSERT_EQ(results.size(), 4);
    ASSERT_EQ(results[0], "High 2");
    ASSERT_EQ(results[1], "High 4");
    ASSERT_EQ(results[2], "Normal 1");
    ASSERT_EQ(results[3], "Normal 3");
}

TEST(PriorityQueueTest, Pop_blocks_when_empty) {
    std::map<TaskPriority, QueueOptions> options;
    options[TaskPriority::Normal] = {false, std::nullopt};
    PriorityQueue priority_queue(options);

    std::atomic<bool> pop_blocked = false;
    std::jthread pop_thread([&]() {
        pop_blocked = true;
        pop_blocked.notify_all();
        priority_queue.pop();
        pop_blocked = false;
        pop_blocked.notify_all();
    });

    pop_blocked.wait(false);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ASSERT_TRUE(pop_blocked);

    priority_queue.push(TaskPriority::Normal, [] {});

    pop_blocked.wait(true);
    ASSERT_FALSE(pop_blocked);
}

TEST(PriorityQueueTest, Shutdown_unblocks_pop) {
    std::map<TaskPriority, QueueOptions> options;
    options[TaskPriority::Normal] = {false, std::nullopt};
    PriorityQueue priority_queue(options);

    std::atomic<bool> pop_blocked = false;
    std::jthread pop_thread([&]() {
        pop_blocked = true;
        pop_blocked.notify_all();
        ASSERT_FALSE(priority_queue.pop().has_value());
        pop_blocked = false;
        pop_blocked.notify_all();
    });

    pop_blocked.wait(false);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ASSERT_TRUE(pop_blocked);

    priority_queue.shutdown();

    pop_blocked.wait(true);
    ASSERT_FALSE(pop_blocked);
}

TEST(PriorityQueueTest, MultiThread_push_and_pop) {
    std::map<TaskPriority, QueueOptions> options;
    options[TaskPriority::High] = {false, std::nullopt};
    options[TaskPriority::Normal] = {false, std::nullopt};
    PriorityQueue priority_queue(options);

    const int high_prio_tasks = 200;
    const int normal_prio_tasks = 200;
    std::atomic<int> high_prio_processed = 0;
    std::atomic<int> normal_prio_processed = 0;

    auto push_func = [&]() {
        for (int i = 0; i < high_prio_tasks; ++i) {
            priority_queue.push(TaskPriority::High, [&] { high_prio_processed++; });
        }
        for (int i = 0; i < normal_prio_tasks; ++i) {
            priority_queue.push(TaskPriority::Normal, [&] { normal_prio_processed++; });
        }
    };

    auto pop_func = [&]() {
        for (size_t i = 0; i < high_prio_tasks + normal_prio_tasks; i++) {
            auto task = priority_queue.pop();
            ASSERT_TRUE(task.has_value());
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

    ASSERT_EQ(high_prio_processed, high_prio_tasks * 3);
    ASSERT_EQ(normal_prio_processed, normal_prio_tasks * 3);
}

TEST(PriorityQueueTest, Bounded_and_Unbounded_queues_work_together) {
    std::map<TaskPriority, QueueOptions> options = {{TaskPriority::High, {true, 2}},
                                                    {TaskPriority::Normal, {false, std::nullopt}}};

    PriorityQueue priority_queue(options);
    std::atomic<int> counter = 0;

    priority_queue.push(TaskPriority::High, [&]() { counter += 20; });
    priority_queue.push(TaskPriority::High, [&]() { counter += 20; });

    priority_queue.push(TaskPriority::Normal, [&]() { counter += 1; });
    priority_queue.push(TaskPriority::Normal, [&]() { counter += 1; });

    auto taskBounded_1 = priority_queue.pop();
    ASSERT_TRUE(taskBounded_1.has_value());
    taskBounded_1.value()();

    auto taskBounded_2 = priority_queue.pop();
    ASSERT_TRUE(taskBounded_2.has_value());
    taskBounded_2.value()();

    auto taskUnbounded_1 = priority_queue.pop();
    ASSERT_TRUE(taskUnbounded_1.has_value());
    taskUnbounded_1.value()();

    auto taskUnbounded_2 = priority_queue.pop();
    ASSERT_TRUE(taskUnbounded_2.has_value());
    taskUnbounded_2.value()();

    ASSERT_EQ(counter, 42);
}

}  // namespace dispatcher::queue