#include "task_dispatcher.hpp"
#include <atomic>
#include <gtest/gtest.h>
#include <string>
#include <thread>
#include <vector>

namespace dispatcher {

using namespace std::chrono_literals;

TEST(TaskDispatcherTest, Constructor_with_default_options) {
    TaskDispatcher dispatcher(2);
    SUCCEED();
}

TEST(TaskDispatcherTest, Constructor_with_custom_options) {
    std::map<TaskPriority, queue::QueueOptions> custom_options;
    custom_options[TaskPriority::High] = {true, 5};
    custom_options[TaskPriority::Normal] = {false, std::nullopt};

    TaskDispatcher dispatcher(1, custom_options);
    SUCCEED();
}

TEST(TaskDispatcherTest, Schedule_executes_tasks) {
    const size_t thread_count = 2;
    TaskDispatcher dispatcher(thread_count);

    std::atomic<int> counter = 0;
    const int num_tasks = 50;

    for (int i = 0; i < num_tasks; ++i) {
        dispatcher.schedule(TaskPriority::Normal, [&]() {
            counter++;
            std::this_thread::sleep_for(3ms);
        });
    }

    while (counter.load() < num_tasks) {
        std::this_thread::sleep_for(10ms);
    }

    ASSERT_EQ(counter.load(), num_tasks);
}

TEST(TaskDispatcherTest, Schedule_respects_priority) {
    const size_t thread_count = 1;
    TaskDispatcher dispatcher(thread_count);

    std::vector<std::string> results;
    std::mutex results_mutex;

    dispatcher.schedule(TaskPriority::Normal, [&]() {
        std::lock_guard<std::mutex> lock(results_mutex);
        results.push_back("Normal 1");
    });
    dispatcher.schedule(TaskPriority::High, [&]() {
        std::lock_guard<std::mutex> lock(results_mutex);
        results.push_back("High 2");
    });
    dispatcher.schedule(TaskPriority::Normal, [&]() {
        std::lock_guard<std::mutex> lock(results_mutex);
        results.push_back("Normal 3");
    });
    dispatcher.schedule(TaskPriority::High, [&]() {
        std::lock_guard<std::mutex> lock(results_mutex);
        results.push_back("High 4");
    });

    int expected_total_tasks = 4;
    while (results.size() < expected_total_tasks) {
        std::this_thread::sleep_for(10ms);
    }

    ASSERT_EQ(results.size(), expected_total_tasks);

    ASSERT_EQ(results[0], "High 2");
    ASSERT_EQ(results[1], "High 4");
    ASSERT_EQ(results[2], "Normal 1");
    ASSERT_EQ(results[3], "Normal 3");
}

TEST(TaskDispatcherTest, Destructor_shutdown_gracefully) {
    std::atomic<int> tasks_executed = 0;
    const int num_tasks = 100;

    {
        TaskDispatcher dispatcher(2);

        for (int i = 0; i < num_tasks; ++i) {
            dispatcher.schedule(TaskPriority::Normal, [&]() {
                tasks_executed++;
                std::this_thread::sleep_for(3ms);
            });
        }
    }

    ASSERT_EQ(tasks_executed.load(), num_tasks);
}

TEST(TaskDispatcherTest, BoundedQueue_default_config_works_as_expected) {
    const size_t thread_count = 1;
    TaskDispatcher dispatcher(thread_count);

    std::atomic<int> counter = 0;

    for (int i = 0; i < 1000; ++i) {
        dispatcher.schedule(TaskPriority::High, [&]() { counter++; });
    }

    {
        std::jthread thread_with_push_blocked(
            [&]() { dispatcher.schedule(TaskPriority::High, [&]() { counter += 10000; }); });
    }

    while (counter.load() < 1000 + 10000) {
        std::this_thread::sleep_for(10ms);
    }

    ASSERT_EQ(counter.load(), 1000 + 10000);
}

}  // namespace dispatcher