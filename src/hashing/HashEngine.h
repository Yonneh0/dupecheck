#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <bcrypt.h>
#include <mutex>
#include <vector>
#include "../core/FileInfo.h"
#include "xxhash/xxhash.h"

extern BCRYPT_ALG_HANDLE g_bcrypt_alg_;
extern std::once_flag s_init_flag;

class HashEngine {
public:
    static void init_bcrypt();
    static void cleanup();
    static BCRYPT_ALG_HANDLE get_alg_handle() { return g_bcrypt_alg_; }
    static HashResult compute(const wchar_t* path);

private:
    inline static bool s_initialized_ = false;
};

inline void HashEngine::init_bcrypt() {
    if (s_initialized_) return;
    std::call_once(s_init_flag, []() {
        NTSTATUS status = BCryptOpenAlgorithmProvider(&g_bcrypt_alg_, BCRYPT_SHA256_ALGORITHM, nullptr, 0);
        s_initialized_ = (status == ERROR_SUCCESS);
    });
}

inline void HashEngine::cleanup() {
    if (g_bcrypt_alg_) {
        BCryptCloseAlgorithmProvider(g_bcrypt_alg_, 0);
        g_bcrypt_alg_ = nullptr;
    }
    s_initialized_ = false;
}

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