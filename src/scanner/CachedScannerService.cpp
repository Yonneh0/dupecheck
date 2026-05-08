#include "CachedScannerService.h"
#include "../hashing/HashEngine.h"

CachedScannerService::CachedScannerService(const std::wstring& db_path) : db_(db_path) {}

bool CachedScannerService::init() {
    return db_.init();
}

std::vector<FileInfo> CachedScannerService::scan(const wchar_t* path) {
    // Step 1: Enumerate all files in the directory.
    std::vector<PathUtils::FileEntry> current_files;
    PathUtils::enumerate_files(path, current_files);

    // Get existing cache entries.
    auto cached = db_.get_cached_files();

    // Create a set of current paths for quick lookup.
    std::unordered_set<std::wstring> current_paths;
    for (auto& f : current_files) {
        current_paths.insert(f.path);
    }

    // Step 2: Remove deleted files from cache.
    db_.remove_deleted_files(current_paths);

    // Step 3: Incremental scan.
    std::vector<FileInfo> results;
    
    for (auto& entry : current_files) {
        bool found = false;
        
        // Check if file exists in cache with same size + mtime.
        for (auto& cached_file : cached) {
            if (cached_file.path == entry.path && 
                cached_file.size == entry.size &&
                cached_file.mtime == entry.mtime) {
                
                // Use cached hash values.
                results.push_back(cached_file);
                found = true;
                break;
            }
        }

        if (!found) {
            // Compute hashes for new or modified file.
            HashResult hr = HashEngine::compute(entry.path.c_str());

            FileInfo fi{entry.path, entry.size, entry.mtime};
            fi.xxhash = hr.xxhash;
            std::copy(std::begin(hr.sha256), std::end(hr.sha256), std::begin(fi.sha256));

            results.push_back(std::move(fi));

            // Save to cache (upsert).
            db_.upsert_file(fi, static_cast<long long>(std::time(nullptr)));
        }
    }

    return results;
}