#include "CachedScannerService.h"
#include "../hashing/HashEngine.h"

CachedScannerService::CachedScannerService(const std::wstring& db_path) : manager_(db_path) {}

bool CachedScannerService::init() { return manager_.init(); }

std::vector<FileInfo> CachedScannerService::scan(const wchar_t* path) {
    auto cached = manager_.get_cached_files();

    std::vector<PathUtils::FileInfo> current_entries;
    PathUtils::enumerate_files(path, current_entries);

    // Remove files that no longer exist.
    std::vector<std::wstring> current_paths(current_entries.size());
    for (size_t i = 0; i < current_entries.size(); ++i) {
        current_paths[i] = current_entries[i].path;
    }
    manager_.remove_deleted_files(current_paths);

    // Merge cached and new entries.
    std::vector<FileInfo> results;
    for (auto& entry : current_entries) {
        bool found = false;
        for (const auto& cf : cached) {
            if (cf.path == entry.path && cf.size == entry.size && cf.mtime == entry.mtime) {
                results.push_back(cf);
                found = true;
                break;
            }
        }
        if (!found) {
            HashResult hr = HashEngine::compute(entry.path.c_str());
            FileInfo fi{entry.path, entry.size, entry.mtime, hr.xxhash, hr.sha256};
            manager_.upsert_file(fi, static_cast<long long>(std::time(nullptr)));
            results.push_back(std::move(fi));
        }
    }
    return results;
}