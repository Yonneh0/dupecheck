#pragma once
#include <windows.h>
#include <bcrypt.h>
#include <vector>
#include "../core/FileInfo.h"

// Global BCrypt state — initialized on first use via std::call_once.
extern HMODULE g_bcrypt_handle;
extern BCRYPT_ALG_HANDLE g_bcrypt_alg_;
extern std::once_flag s_init_flag;

/// Computes XxHash32 and SHA256 for files using single-pass I/O.
class HashEngine {
public:
    /// Initialize the BCrypt SHA256 algorithm handle (thread-safe, called once).
    static void init_bcrypt();

    /// Close the BCrypt algorithm handle. Call at shutdown.
    static void cleanup();

    /// Get the SHA256 algorithm handle — valid after init_bcrypt().
    static BCRYPT_ALG_HANDLE get_alg_handle() { return g_bcrypt_alg_; }

    /// Compute XxHash32 and SHA256 for a single file in one I/O pass.
    static HashResult compute(const wchar_t* path);

    /// Batch hash files using deferred async tasks (one per file).
    static void compute_batch(const std::vector<std::wstring>& paths, std::vector<HashResult>& out);

private:
    inline static bool s_initialized_ = false;
};

// Initialize BCrypt SHA256 algorithm handle. Called once before any hashing.
inline void HashEngine::init_bcrypt() {
    if (s_initialized_) return;
    std::call_once(s_init_flag, []() {
        NTSTATUS status = BCryptOpenAlgorithmProvider(&g_bcrypt_alg_, BCRYPT_SHA256_ALGORITHM, nullptr, 0);
        s_initialized_ = (status == ERROR_SUCCESS);
    });
}

// Close the BCrypt algorithm handle. Call at shutdown.
inline void HashEngine::cleanup() {
    if (g_bcrypt_alg_) {
        BCryptCloseAlgorithmProvider(g_bcrypt_alg_, 0);
        g_bcrypt_alg_ = nullptr;
    }
    s_initialized_ = false;
}

// Single-pass SHA256+XxHash32 computation on the given file path.
inline HashResult HashEngine::compute(const wchar_t* path) {
    HashResult result{};
    HANDLE hFile = CreateFileW(
        PathUtils::to_long_path(path).c_str(),
        GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

    if (hFile == INVALID_HANDLE_VALUE) return result;

    LARGE_INTEGER fileSize{};
    GetFileSizeEx(hFile, &fileSize);
    uint64_t total_bytes = static_cast<uint64_t>(fileSize.QuadPart);

    BCRYPT_HASH_HANDLE hSha256{};
    NTSTATUS status = BCryptCreateHash(g_bcrypt_alg_, &hSha256, nullptr, 0, nullptr, 0, 0);
    if (status != ERROR_SUCCESS) { CloseHandle(hFile); return result; }

    uint32_t xxhash_state = 0;
    char buffer[HASH_BUFFER_SIZE]{};

    while (total_bytes > 0) {
        DWORD bytes_to_read = static_cast<DWORD>(std::min(HASH_BUFFER_SIZE, total_bytes));
        DWORD bytesRead = 0;
        if (!ReadFile(hFile, buffer, bytes_to_read, &bytesRead, nullptr)) break;
        if (bytesRead == 0) break;

        xxhash_state = XXH32(buffer, bytesRead, xxhash_state);
        BCryptHashData(hSha256, reinterpret_cast<uint8_t*>(buffer), bytesRead, 0);
        total_bytes -= bytesRead;
    }

    result.xxhash = xxhash_state;
    BCryptFinishHash(hSha256, result.sha256.data(), static_cast<DWORD>(result.sha256.size()), 0);
    BCryptDestroyHash(hSha256);
    CloseHandle(hFile);
    return result;
}
