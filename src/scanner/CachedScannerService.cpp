#include <unordered_map>
#include "CachedScannerService.h"
#include "../hashing/HashEngine.h"
#include "../utils/DbPath.h"

bool CachedScannerService::init() {
    std::wstring db_path = get_default_db_path();
    manager_ = DatabaseManager{db_path};
    return manager_.init();
}

std::vector<FileInfo> CachedScannerService::scan(const wchar_t* path) {
    auto cached_files = manager_.get_cached_files();

    // Build a hash map from path to cached entry for O(1) lookups.
    std::unordered_map<std::wstring, const FileInfo*> cached_map;
    cached_map.reserve(cached_files.size());
    for (const auto& f : cached_files) {
        cached_map[f.path] = &f;
    }

    std::vector<FileInfo> current_entries;
    PathUtils::enumerate_files(path, current_entries);

    // Collect paths of files found on disk.
    std::vector<std::wstring> current_paths;
    current_paths.reserve(current_entries.size());
    for (const auto& e : current_entries) {
        current_paths.push_back(e.path);
    }
    manager_.remove_deleted_files(current_paths);

    const long long now_seconds = static_cast<long long>(std::time(nullptr));
    std::vector<FileInfo> results;
    results.reserve(current_entries.size());

    for (const auto& entry : current_entries) {
        auto it = cached_map.find(entry.path);
        if (it != cached_map.end() && it->second->size == entry.size && it->second->mtime == entry.mtime) {
            // Cache hit: file unchanged.
            results.push_back(*it->second);
        } else {
            // Re-hash and upsert.
            HashResult hr = HashEngine::compute(entry.path.c_str());
            FileInfo fi{entry.path, entry.size, entry.mtime, hr.xxhash, hr.sha256};
            manager_.upsert_file(fi, now_seconds);
            results.push_back(std::move(fi));
        }
    }
    return results;
}
