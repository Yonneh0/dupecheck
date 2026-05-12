#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <array>
#include <cstdint>

/// Offset from FILETIME epoch (Jan 1, 1601) to Unix epoch (Jan 1, 1970) in seconds.
/// FILETIME is measured in 100-nanosecond intervals since Jan 1, 1601; dividing by 10,000,000 converts to seconds.
constexpr long long EPOCH_OFFSET = 13477420800LL;

/// Read buffer size for hashing I/O — 64 KB is optimal for sequential file reads on Windows.
constexpr size_t HASH_BUFFER_SIZE = 65536;

using Sha256 = std::array<uint8_t, 32>;
using XxHash32 = uint32_t;

struct HashResult {
    XxHash32 xxhash = 0;
    Sha256 sha256{};
};

/// File metadata used throughout the application.
struct FileInfo {
    std::wstring path;
    uint64_t size = 0;
    long long mtime = 0;
    XxHash32 xxhash = 0;
    Sha256 sha256{};
};

using FileEntry = FileInfo;

namespace PathUtils {

inline std::wstring utf8_to_wide(const std::string& s) {
    if (s.empty()) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    if (len == 0) return {};
    std::wstring result(len - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &result[0], len);
    return result;
}

inline std::string wide_to_utf8(const std::wstring& w) {
    if (w.empty()) return {};
    int len = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (len == 0) return {};
    std::string result(len - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, &result[0], len, nullptr, nullptr);
    return result;
}

/// Convert to Windows long path format (\\?\ prefix) for paths >260 chars.
inline std::wstring to_long_path(const std::wstring& p) {
    if (p.length() > 260 && p.substr(0, 4) != L"\\\\?\\") {
        return L"\\\\?\\" + p;
    }
    return p;
}

inline std::string get_extension(const std::wstring& path) {
    std::filesystem::path fp(path);
    auto ext = fp.extension().string();
    if (!ext.empty() && ext[0] == '.') ext.erase(ext.begin());
    for (auto& c : ext) c = static_cast<char>(std::tolower(c));
    return ext;
}

inline std::wstring get_name_without_ext(const std::wstring& path) {
    return std::filesystem::path(path).stem().string();
}

inline std::wstring get_parent_dir(const std::wstring& path) {
    auto p = std::filesystem::path(path).parent_path();
    if (p.empty()) return L"";
    return p.wstring();
}

/// Check whether the given path refers to a file.
inline bool is_file(const std::wstring& path) {
    DWORD attrs = GetFileAttributesW(to_long_path(path).c_str());
    return (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY) == 0);
}

inline uint64_t get_file_size(const std::wstring& path) {
    WIN32_FILE_ATTRIBUTE_DATA info;
    if (!GetFileAttributesExW(to_long_path(path).c_str(), GetFileExInfoStandard, &info)) return 0;
    ULARGE_INTEGER size;
    size.QuadPart = static_cast<uint64_t>(info.nFileSizeHigh) << 32 | info.nFileSizeLow;
    return size.QuadPart;
}

inline long long get_file_mtime(const std::wstring& path) {
    WIN32_FILE_ATTRIBUTE_DATA info;
    if (!GetFileAttributesExW(to_long_path(path).c_str(), GetFileExInfoStandard, &info)) return 0;
    ULARGE_INTEGER ft;
    ft.LowPart = info.ftLastWriteTime.dwLowDateTime;
    ft.HighPart = info.ftLastWriteTime.dwHighDateTime;
    return static_cast<long long>(ft.QuadPart / 10000000) - EPOCH_OFFSET;
}

/// Recursively enumerate all files under `dir`, appending them to `out`.
inline void enumerate_files(const std::wstring& dir, std::vector<FileInfo>& out) {
    if (dir.empty()) return;

    auto long_dir = to_long_path(dir);
    bool is_root_drive = (long_dir.length() <= 4 && long_dir.back() == L':');
    std::wstring search_pattern = (is_root_drive) ? long_dir + L"\\*.*" : long_dir + L"\\";

    WIN32_FIND_DATAW find_data;
    HANDLE hFind = FindFirstFileExW(search_pattern.c_str(),
                                   FindExInfoStandard, &find_data,
                                   FindExSearchNameMatch, nullptr, 0);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        if (wcscmp(find_data.cFileName, L".") == 0 ||
            wcscmp(find_data.cFileName, L"..") == 0) continue;

        std::wstring full_path = long_dir + L"\\" + find_data.cFileName;
        if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            enumerate_files(full_path, out);
        } else {
            FileInfo entry;
            entry.path = full_path;
            entry.size = static_cast<uint64_t>(find_data.nFileSizeHigh * 0x100000000ull) + find_data.nFileSizeLow;
            ULARGE_INTEGER ft;
            ft.LowPart = find_data.ftLastWriteTime.dwLowDateTime;
            ft.HighPart = find_data.ftLastWriteTime.dwHighDateTime;
            entry.mtime = static_cast<long long>(ft.QuadPart / 10000000) - EPOCH_OFFSET;
            out.push_back(std::move(entry));
        }
    } while (FindNextFileW(hFind, &find_data));

    FindClose(hFind);
}

inline std::wstring get_duplicates_subfolder(const std::wstring& dir) {
    return dir + L"\\duplicates";
}

} // namespace PathUtils