#pragma once
#include "../core/FileInfo.h"
#include <bcrypt.h>
#include <windows.h>
#include <cstdint>
#include <array>
#include <vector>
#include <string>
#include <future>
#include <thread>
#include <mutex>
#include <functional>
#include <memory>

// Compute both XxHash32 and SHA256 in a single I/O pass over the file.
struct HashResult {
    uint32_t xxhash;
    std::array<uint8_t, 32> sha256;
};

class HashEngine {
public:
    // Compute hashes for a single file (single-pass).
    static HashResult compute(const wchar_t* path);

    // Parallel batch hash computation using thread pool.
    static void compute_batch(const std::vector<std::wstring>& paths,
                              std::vector<HashResult>& out);

private:
    // Initialize bcrypt SHA256 provider (thread-safe, called once).
    static void init_bcrypt();

    // Cleanup bcrypt handles at process exit.
    static void cleanup();

    // Static member — the SHA256 bcrypt algorithm handle.
    inline static BCRYPT_ALG_HANDLE g_bcrypt_alg_{nullptr};
    static bool s_initialized;
    static std::once_flag s_init_flag;
};

class ThreadPool {
public:
    explicit ThreadPool(int num_threads = 0);
    ~ThreadPool();

    // Submit a work item and get future.
    template<typename F>
    std::future<void> submit(F&& f) {
        auto task = std::make_shared<std::packaged_task<void()>>(std::forward<F>(f));
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            tasks_.push([task]() { (*task)(); });
            active_tasks_++;
        }
        cv_.notify_one();
        return task->get_future();
    }

    void wait_all();

private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex queue_mutex_;
    std::condition_variable cv_;
    std::atomic<bool> stop_{false};
    int active_tasks_ = 0;
    std::mutex active_mutex_;
    std::condition_variable done_cv_;

    void worker_loop();
};