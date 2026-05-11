#pragma once

#include <atomic>
#include <functional>
#include <future>
#include <mutex>
#include <thread>
#include <vector>
#include <windows.h>
#include "../core/FileInfo.h"

class HashEngine {
public:
    static void init_bcrypt();
    static void cleanup();
    static HashResult compute(const wchar_t* path);
    static void compute_batch(const std::vector<std::wstring>& paths, std::vector<HashResult>& out);
    static BCRYPT_ALG_HANDLE get_alg_handle() { return g_bcrypt_alg_; }

private:
    inline static BCRYPT_ALG_HANDLE g_bcrypt_alg_ = nullptr;
    inline static bool s_initialized = false;
    inline static std::once_flag s_init_flag;
};

class ThreadPool {
public:
    explicit ThreadPool(int num_threads);
    ~ThreadPool();

    template<class F, class... Args>
    auto submit(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
        using RetType = decltype(f(args...));
        auto task = std::make_shared<std::packaged_task<RetType()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        std::future<RetType> result = task->get_future();
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            if (stop_) throw std::runtime_error("submit on stopped ThreadPool");
            tasks_.emplace([task]() { (*task)(); });
        }
        return result;
    }

private:
    void worker_loop();
    bool stop_ = false;
    std::queue<std::function<void()>> tasks_;
    std::mutex queue_mutex_;
    int active_tasks_ = 0;
    std::condition_variable cv_;
    std::condition_variable done_cv_;
    std::vector<std::thread> workers_;

public:
    void wait_all();
};
