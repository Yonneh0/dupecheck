#include "HashEngine.h"
#include <future>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <queue>
#include <atomic>

// ThreadPool — work-queue based parallel executor.
class ThreadPool {
public:
    explicit ThreadPool(int num_threads) {
        if (num_threads <= 0) {
            SYSTEM_INFO info;
            GetSystemInfo(&info);
            num_threads = static_cast<int>(info.dwNumberOfProcessors);
        }

        for (int i = 0; i < num_threads; ++i) {
            workers_.emplace_back([this]() { worker_loop(); });
        }
    }

    ~ThreadPool() {
        stop_ = true;
        cv_.notify_all();
        for (auto& w : workers_) {
            if (w.joinable()) w.join();
        }
    }

    void worker_loop() {
        while (true) {
            std::function<void()> task;
            bool has_task = false;
            {
                std::unique_lock<std::mutex> lock(queue_mutex_);
                cv_.wait(lock, [this]() { return stop_ || !tasks_.empty(); });
                if (stop_ && tasks_.empty()) return;
                task = std::move(tasks_.front());
                tasks_.pop();
            }

            try { task(); } catch (...) {}

            {
                std::lock_guard<std::mutex> lock(active_mutex_);
                if (--active_tasks_ == 0) done_cv_.notify_one();
            }
        }
    }

    // Block until all submitted tasks complete.
    void wait_all() {
        while (true) {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            if (tasks_.empty() && active_tasks_ == 0) return;
            done_cv_.wait_for(lock, std::chrono::milliseconds(50));
        }
    }

private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex queue_mutex_;
    std::condition_variable cv_;
    int active_tasks_ = 0;
    std::atomic<bool> stop_{false};
    std::mutex active_mutex_;
    std::condition_variable done_cv_;
};

void HashEngine::compute_batch(const std::vector<std::wstring>& paths, std::vector<HashResult>& out) {
    init_bcrypt();

    SYSTEM_INFO info{};
    GetSystemInfo(&info);
    int num_threads = (info.dwNumberOfProcessors > 0) ? static_cast<int>(info.dwNumberOfProcessors - 1) : 3;

    // Use std::async for true parallel execution.
    std::vector<std::future<HashResult>> futures(paths.size());
    out.resize(paths.size());

    for (size_t i = 0; i < paths.size(); ++i) {
        const auto& p = paths[i];
        futures[i] = std::async(std::launch::async, [&p]() -> HashResult { return compute(p.c_str()); });
    }

    for (size_t i = 0; i < paths.size(); ++i) {
        out[i] = futures[i].get();
    }
}