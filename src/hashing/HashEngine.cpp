#include "HashEngine.h"
#include <future>

// Define the external globals declared in HashEngine.h — single definition per TU.
HMODULE g_bcrypt_handle = nullptr;
BCRYPT_ALG_HANDLE g_bcrypt_alg_ = nullptr;
bool s_initialized = false;
std::once_flag s_init_flag{};

void HashEngine::compute_batch(const std::vector<std::wstring>& paths, std::vector<HashResult>& out) {
    init_bcrypt();

    SYSTEM_INFO info{};
    GetSystemInfo(&info);
    int num_threads = (info.dwNumberOfProcessors > 0) ? static_cast<int>(info.dwNumberOfProcessors - 1) : 3;

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