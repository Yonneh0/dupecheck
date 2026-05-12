#include "HashEngine.h"
#include <future>
#include <algorithm>

// Global BCrypt state — initialized via std::call_once on first use.
HMODULE g_bcrypt_handle = nullptr;
BCRYPT_ALG_HANDLE g_bcrypt_alg_ = nullptr;
std::once_flag s_init_flag{};

void HashEngine::compute_batch(const std::vector<std::wstring>& paths, std::vector<HashResult>& out) {
    init_bcrypt();

    // One async task per file — suitable for I/O-bound work.
    std::vector<std::future<HashResult>> futures(paths.size());
    out.resize(paths.size());

    for (size_t i = 0; i < paths.size(); ++i) {
        const auto& p = paths[i];
        futures[i] = std::async(std::launch::async, [&p]() -> HashResult { return compute(p.c_str()); });
    }

    // Collect results.
    for (size_t i = 0; i < paths.size(); ++i) {
        out[i] = futures[i].get();
    }
}