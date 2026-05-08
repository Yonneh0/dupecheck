#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <windows.h>
#include <cstdint>

// Unix epoch offset: FILETIME ticks (since Jan 1, 1601 UTC) minus seconds since Jan 1, 1970 UTC.
constexpr long long EPOCH_OFFSET = 13477420800LL;

/// Alias types used across the codebase. Sha256 is a fixed-size 32-byte array; XxHash32 is 32-bit.
using Sha256 = std::array<uint8_t, 32>;
using XxHash32 = uint32_t;

/// Utility functions for Windows file paths (UTF-16).
namespace PathUtils {

/// Convert UTF-8 std::string to Windows-wide string (UTF-16).
inline std::wstring utf8_to_wide(const std::string& s) {
        if (s.empty()) return {};
        int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
        if (len == 0) return {};
        std::wstring result(len - 1, 0);
        MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &result[0], len);
        return result;
    }

/// Convert Windows-wide string (UTF-16) to UTF-8 std::string.
inline std::string wide_to_utf8(const std::wstring& w) {
        if (w.empty()) return {};
        int len = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (len == 0) return {};
        std::string result(len - 1, 0);
        WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, &result[0], len, nullptr, nullptr);
        return result;
    }

/// Convert a relative or short path to the Windows "\\?\" long-path format, which supports paths > 260 characters.
inline std::wstring to_long_path(const std::wstring& p) {
        if (p.length() > 260 && p.substr(0, 4) != L"\\\\?\\") {
            return L"\\\\?\\" + p;
        }
        return p;
    }

/// Extract the file extension from a path — returns lowercase string without leading dot (e.g. "jpg").
inline std::string get_extension(const std::wstring& path) {
        std::filesystem::path fp(path);
        auto ext = fp.extension().string();
        if (!ext.empty() && ext[0] == '.') {
            ext.erase(ext.begin());
        }
        for (auto& c : ext) c = static_cast<char>(std::tolower(c));
        return ext;
    }

/// Return the file name stem, excluding any extension.
inline std::wstring get_name_without_ext(const std::wstring& path) {
        std::filesystem::path fp(path);
        return fp.stem().string();
    }

/// Return the parent directory of a path (empty string for root paths).
inline std::wstring get_parent_dir(const std::wstring& path) {
        std::filesystem::path fp(path);
        auto p = fp.parent_path();
        if (p.empty()) {
            return L"";
        }
        return p.wstring();
    }

/// Return true if the given path refers to an existing file on disk.
inline bool is_file(const std::wstring& path) {
        DWORD attrs = GetFileAttributesW(to_long_path(path).c_str());
        return (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY) == 0);
    }

/// Return the file size in bytes, or 0 if the file doesn't exist.
inline uint64_t get_file_size(const std::wstring& path) {
        WIN32_FILE_ATTRIBUTE_DATA info;
        if (!GetFileAttributesExW(to_long_path(path).c_str(), GetFileExInfoStandard, &info))
            return 0;
        ULARGE_INTEGER size;
        size.QuadPart = 0;
        size.LowPart = info.nFileSizeLow;
        size.HighPart = info.nFileSizeHigh;
        return size.QuadPart;
    }

/// Return the last-write time of a file, expressed as seconds-since-epoch (UTC).
inline long long get_file_mtime(const std::wstring& path) {
        WIN32_FILE_ATTRIBUTE_DATA info;
        if (!GetFileAttributesExW(to_long_path(path).c_str(), GetFileExInfoStandard, &info))
            return 0;

        // FILETIME is in 100-nanosecond intervals since Jan 1, 1601 (UTC)
        ULARGE_INTEGER ft;
        ft.LowPart = info.ftLastWriteTime.dwLowDateTime;
        ft.HighPart = info.ftLastWriteTime.dwHighDateTime;

        // Convert to seconds since epoch (Jan 1, 1970 UTC)
        constexpr long long EPOCH_OFFSET = 13477420800LL;
        return static_cast<long long>(ft.QuadPart / 10000000) - EPOCH_OFFSET;
    }

/// Metadata for a single file entry, used during directory enumeration.
struct FileEntry {
    std::wstring path;  /// Full Windows path to the file.
    uint64_t size;      /// File size in bytes.
    long long mtime;    /// Last-write time, seconds since epoch (UTC).
};

/// Enumerate all files recursively under `dir`, appending results to `out`.

    inline void enumerate_files(const std::wstring& dir, std::vector<FileEntry>& out) {
        if (dir.empty()) return;

        auto long_dir = to_long_path(dir);

        // Handle root directory case for drives like "C:\"
        bool is_root_drive = (long_dir.length() <= 4 && long_dir.back() == L':');
        std::wstring search_pattern = (is_root_drive) ? dir + L"\\*.*" : long_dir + L"\\*";

        WIN32_FIND_DATAW find_data;
        HANDLE hFind = FindFirstFileExW(search_pattern.c_str(),
                                         FindExInfoStandard,
                                         &find_data,
                                         FindExSearchNameMatch,
                                         nullptr, 0);

        if (hFind == INVALID_HANDLE_VALUE) return;

        do {
            // Skip . and ..
            if (wcscmp(find_data.cFileName, L".") == 0 ||
                wcscmp(find_data.cFileName, L"..") == 0)
                continue;

            std::wstring full_path = long_dir + L"\\" + find_data.cFileName;

            if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                // Recurse into subdirectory.
                enumerate_files(full_path, out);
            } else {
                FileEntry entry;
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

/// Return the path of a "duplicates" subfolder inside the given directory.
inline std::wstring get_duplicates_subfolder(const std::wstring& dir) {
        return dir + L"\\duplicates";
    }
} // namespace PathUtils