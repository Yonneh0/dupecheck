#include <windows.h>
#include "CachedScannerService.h"
#include "../hashing/HashEngine.h"

// The database path is set via the init() method which uses get_default_db_path().
bool CachedScannerService::init() {
    // Use the default DB path (same as ImGuiView).
    wchar_t appdata[MAX_PATH];
    DWORD len = ExpandEnvironmentStringsW(L"%APPDATA%", appdata, ARRAYSIZE(appdata));
    if (len == 0 || len > static_cast<DWORD>(ARRAYSIZE(appdata))) return false;

    std::wstring db_path = std::wstring(appdata) + L"\\DupeCheck\\dupecheck.db";
    auto dir_pos = db_path.find_last_of(L'\\');
    if (dir_pos != std::wstring::npos) {
        CreateDirectoryW(db_path.substr(0, dir_pos).c_str(), nullptr);
    }
    manager_ = DatabaseManager{db_path};
    return manager_.init();
}

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