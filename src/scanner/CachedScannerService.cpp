#include "CachedScannerService.h"
#include <unordered_set>
#include <unordered_map>
#include "../hashing/HashEngine.h"

CachedScannerService::CachedScannerService(const std::wstring& db_path) : db_(db_path) {}

bool CachedScannerService::init() {
    return db_.init();
}

std::vector<FileInfo> CachedScannerService::scan(const wchar_t* path) {
    std::vector<PathUtils::FileEntry> current_files;
    PathUtils::enumerate_files(path, current_files);

    auto cached = db_.get_cached_files();

    std::unordered_set<std::wstring> current_paths;
    for (auto& f : current_files) {
        current_paths.insert(f.path);
    }

    db_.remove_deleted_files(current_paths);

    std::unordered_map<std::wstring, FileInfo> cache_map;
    for (const auto& cf : cached) {
        cache_map[cf.path] = cf;
    }

    std::vector<FileInfo> results;
    
    for (auto& entry : current_files) {
        bool found = false;
        
        auto it = cache_map.find(entry.path);
        if (it != cache_map.end() && 
            it->second.size == entry.size &&
            it->second.mtime == entry.mtime) {
            
            results.push_back(it->second);
            found = true;
        }

        if (!found) {
            HashResult hr = HashEngine::compute(entry.path.c_str());

            FileInfo fi{entry.path, entry.size, entry.mtime};
            fi.xxhash = hr.xxhash;
            std::copy(std::begin(hr.sha256), std::end(hr.sha256), std::begin(fi.sha256));

            results.push_back(std::move(fi));

            db_.upsert_file(fi, static_cast<long long>(std::time(nullptr)));
        }
    }

    return results;
}