#include "HashEngine.h"
#include <future>
#include <algorithm>

// Global BCrypt state — initialized via std::call_once on first use.
HMODULE g_bcrypt_handle = nullptr;
BCRYPT_ALG_HANDLE g_bcrypt_alg_ = nullptr;
std::once_flag s_init_flag{};

void HashEngine::compute_batch(const std::vector<std::wstring>& paths, std::vector<HashResult>& out) {
    init_bcrypt();

    // Use deferred launch to avoid thread explosion; the OS schedules them.
    std::vector<std::future<HashResult>> futures(paths.size());
    out.resize(paths.size());

    for (size_t i = 0; i < paths.size(); ++i) {
        const auto& p = paths[i];
        futures[i] = std::async(std::launch::deferred, [&p]() -> HashResult { return compute(p.c_str()); });
    }

    // Collect results (each future runs its deferred task when get() is called).
    for (size_t i = 0; i < paths.size(); ++i) {
        out[i] = futures[i].get();
    }
}
