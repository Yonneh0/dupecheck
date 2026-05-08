#include "HashEngine.h"
#include <thread>
#include <future>
#include <chrono>

ThreadPool::ThreadPool(int num_threads) {
    if (num_threads <= 0) {
        SYSTEM_INFO info;
        GetSystemInfo(&info);
        num_threads = static_cast<int>(info.dwNumberOfProcessors) - 1;
    }

    for (int i = 0; i < num_threads; ++i) {
        workers_.emplace_back([this]() { worker_loop(); });
    }
}

ThreadPool::~ThreadPool() {
    stop_ = true;
    cv_.notify_all();
    for (auto& w : workers_) {
        if (w.joinable()) w.join();
    }
}

void ThreadPool::worker_loop() {
    while (true) {
        std::function<void()> task;
        bool has_task = false;
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            cv_.wait(lock, [this]() { return stop_ || !tasks_.empty(); });

            if (stop_ && tasks_.empty()) return;

            task = std::move(tasks_.front());
            tasks_.pop();
            has_task = true;
        }

        try {
            task();
        } catch (...) {
            // Prevent exceptions from killing worker threads.
        }

        // Decrement active tasks after completion and notify wait_all().
        {
            std::lock_guard<std::mutex> lock(active_mutex_);
            if (--active_tasks_ == 0) done_cv_.notify_one();
        }
    }
}

void ThreadPool::wait_all() {
    while (true) {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        if (tasks_.empty() && active_tasks_ == 0) return;
        // Wait for tasks to finish executing, not just dequeue.
        done_cv_.wait_for(lock, std::chrono::milliseconds(50));
    }
}
