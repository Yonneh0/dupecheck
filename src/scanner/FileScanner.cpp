#include "FileScanner.h"
#include "../hashing/HashEngine.h"

// Legacy scanner — delegates to the shared CachedDatabase.
FileScanner::FileScanner(const std::wstring& db_path) : cache_(db_path) {}

bool FileScanner::init() { return cache_.init(); }

std::vector<FileInfo> FileScanner::scan(const wchar_t* path) {
    std::vector<PathUtils::FileInfo> current_entries;
    PathUtils::enumerate_files(path, current_entries);

    auto cached = cache_.get_cached_files();

    // Remove files that no longer exist.
    std::vector<std::wstring> current_paths(current_entries.size());
    for (size_t i = 0; i < current_entries.size(); ++i) {
        current_paths[i] = current_entries[i].path;
    }
    cache_.remove_deleted_files(current_paths);

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
            cache_.upsert_file(fi, static_cast<long long>(std::time(nullptr)));
            results.push_back(std::move(fi));
        }
    }
    return results;
}