#include "HashEngine.h"

BCRYPT_ALG_HANDLE g_bcrypt_alg_ = nullptr;
std::once_flag s_init_flag;

void HashEngine::compute_batch(const std::vector<std::wstring>& paths, std::vector<HashResult>& out) {
    // Initialize BCrypt if not already done.
    init_bcrypt();

    // Simple sequential batch computation — each path is computed independently.
    // This function exists for API compatibility; the primary entry point is compute().
    out.clear();
    out.reserve(paths.size());
    for (const auto& p : paths) {
        out.push_back(compute(p.c_str()));
    }
}