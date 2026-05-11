#include "CachedScannerService.h"
#include <unordered_set>
#include "../hashing/HashEngine.h"

CachedScannerService::CachedScannerService(const std::wstring& db_path) : db_(db_path) {}

bool CachedScannerService::init() { return db_.init(); }

std::vector<FileInfo> CachedScannerService::scan(const wchar_t* path) {
    std::vector<PathUtils::FileEntry> current_files; PathUtils::enumerate_files(path, current_files);
    auto cached = db_.get_cached_files();
    std::unordered_set<std::wstring> current_paths; for (auto& f : current_files) current_paths.insert(f.path);
    db_.remove_deleted_files(current_paths);

    std::vector<FileInfo> results;
    for (auto& entry : current_files) {
        bool found = false;
        for (const auto& cf : cached) {
            if (cf.path == entry.path && cf.size == entry.size && cf.mtime == entry.mtime) { results.push_back(cf); found = true; break; }
        }
        if (!found) {
            HashResult hr = HashEngine::compute(entry.path.c_str());
            FileInfo fi{entry.path, entry.size, entry.mtime, hr.xxhash, hr.sha256};
            db_.upsert_file(fi, static_cast<long long>(std::time(nullptr)));
            results.push_back(std::move(fi));
        }
    }
    return results;
}