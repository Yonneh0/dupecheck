#include "HashEngine.h"
#include <fstream>
#include <mutex>
#include <future>
#include <thread>
#include <algorithm>

constexpr size_t HASH_BUFFER_SIZE = 64 * 1024; // 64KB read buffer

// Static member definitions.
HCRYPTPROV_HANDLE HashEngine::g_bcrypt_prov_{};
BCRYPT_ALG_HANDLE HashEngine::g_bcrypt_alg_{nullptr};
bool HashEngine::s_initialized = false;
std::once_flag HashEngine::s_init_flag;

namespace {
    NTSTATUS init_sha256_provider(BCRYPT_ALG_HANDLE& hAlg) {
        return BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, nullptr, 0);
    }

    NTSTATUS init_aes_provider(BCRYPT_ALG_HANDLE& hAlg) {
        return BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_AES_ALGORITHM, nullptr, 0);
    }
} // anonymous namespace

void HashEngine::init_bcrypt() {
    if (s_initialized) return;

    NTSTATUS status = init_sha256_provider(g_bcrypt_alg_);
    if (status != ERROR_SUCCESS) return;

    s_initialized = true;
}

HashResult HashEngine::compute(const wchar_t* path) {
    HashResult result{};
    // Initialize bcrypt on first use.
    std::call_once(s_init_flag, []() { init_bcrypt(); });

    HANDLE hFile = CreateFileW(
        PathUtils::to_long_path(path).c_str(),
        GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

    if (hFile == INVALID_HANDLE_VALUE) return result;

    LARGE_INTEGER fileSize;
    GetFileSizeEx(hFile, &fileSize);
    uint64_t total_bytes = static_cast<uint64_t>(fileSize.QuadPart);

    BCRYPT_HASH_HANDLE hSha256{};
    NTSTATUS status = BCryptCreateHash(g_bcrypt_alg_, &hSha256, nullptr, 0,
                                        nullptr, 0, 0);
    if (status != ERROR_SUCCESS) {
        CloseHandle(hFile);
        return result;
    }

    uint32_t xxhash_state = XXH32(0, 0);
    // Use char buffer for byte-level hashing (not wchar_t which is 16-bit).
    char buffer[HASH_BUFFER_SIZE];

    while (total_bytes > 0) {
        DWORD bytes_to_read = static_cast<DWORD>(std::min(HASH_BUFFER_SIZE, total_bytes));
        DWORD bytesRead = 0;
        if (!ReadFile(hFile, buffer, bytes_to_read, &bytesRead, nullptr)) break;
        if (bytesRead == 0) break;

        const uint8_t* data = reinterpret_cast<const uint8_t*>(buffer);
        for (DWORD i = 0; i < bytesRead / sizeof(uint32_t); ++i) {
            xxhash_state += static_cast<uint32_t>(data[i * sizeof(uint32_t)]) * XXH_PRIME32_2;
            xxhash_state = XXH_ROTL32(xxhash_state, 13);
            xxhash_state *= XXH_PRIME32_1;
        }

        BCryptHashData(hSha256, reinterpret_cast<uint8_t*>(buffer), bytesRead, 0);

        total_bytes -= bytesRead;
    }

    result.xxhash = xxhash_state ^ static_cast<uint32_t>(total_bytes + fileSize.QuadPart);

    DWORD hashLen = sizeof(result.sha256);
    BCryptFinishHash(hSha256, result.sha256.data(), static_cast<DWORD>(result.sha256.size()), 0);

    BCryptDestroyHash(hSha256);
    CloseHandle(hFile);

    return result;
}

void HashEngine::compute_batch(const std::vector<std::wstring>& paths,
                               std::vector<HashResult>& out) {
    // Ensure bcrypt is initialized before spawning workers.
    std::call_once(s_init_flag, []() { init_bcrypt(); });

    SYSTEM_INFO info;
    GetSystemInfo(&info);
    int num_threads = (info.dwNumberOfProcessors > 0) ?
        static_cast<int>(info.dwNumberOfProcessors - 1) : 3;

    ThreadPool pool(std::max(1, num_threads));

    // Pre-size output vector for thread safety.
    out.resize(paths.size());

    std::vector<std::future<void>> futures;
    futures.reserve(paths.size());

    for (size_t i = 0; i < paths.size(); ++i) {
        const auto& p = paths[i];
        futures.push_back(pool.submit([this, &p, idx = i]() {
            out[idx] = compute(p.c_str());
        }));
    }

    pool.wait_all();
}