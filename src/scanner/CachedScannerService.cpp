#include "CachedScannerService.h"
#include "../hashing/HashEngine.h"

CachedScannerService::CachedScannerService(const std::wstring& db_path) : manager_(db_path) {}

bool CachedScannerService::init() { return manager_.init(); }

std::vector<FileInfo> CachedScannerService::scan(const wchar_t* path) {
    // Load all cached entries from the database.
    auto cached = manager_.get_cached_files();

    // Enumerate current files on disk.
    std::vector<FileInfo> current_entries;
    PathUtils::enumerate_files(path, current_entries);

    // Remove cache entries for deleted files.
    std::vector<std::wstring> current_paths(current_entries.size());
    for (size_t i = 0; i < current_entries.size(); ++i) {
        current_paths[i] = current_entries[i].path;
    }
    manager_.remove_deleted_files(current_paths);

    // Merge: reuse cached metadata for unchanged files, re-hash new/modified ones.
    std::vector<FileInfo> results;
    const long long now_seconds = static_cast<long long>(std::time(nullptr));
    results.reserve(cached.size() + current_entries.size());

    for (const auto& entry : current_entries) {
        bool found = false;
        for (auto it = cached.begin(); it != cached.end(); ++it) {
            if (it->path == entry.path && it->size == entry.size && it->mtime == entry.mtime) {
                results.push_back(*it);
                cached.erase(it); // avoid re-checking this cache entry.
                found = true;
                break;
            }
        }
        if (!found) {
            HashResult hr = HashEngine::compute(entry.path.c_str());
            FileInfo fi{entry.path, entry.size, entry.mtime, hr.xxhash, hr.sha256};
            manager_.upsert_file(fi, now_seconds);
            results.push_back(std::move(fi));
        }
    }
    return results;
}