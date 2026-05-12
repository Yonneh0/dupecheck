#pragma once
#include <string>
#include "../core/FileInfo.h"
#include <sqlite3.h>

// Internal SQLite cache used by CachedScannerService.
class CachedDatabase {
public:
    explicit CachedDatabase(const std::wstring& db_path) : db_path_(db_path) {}
    bool init();
    std::vector<FileInfo> get_cached_files() const;
    bool upsert_file(const FileInfo& info, long long last_scan_seconds);
    bool remove_deleted_files(const std::vector<std::wstring>& current_paths);

private:
    static constexpr const char* SCHEMA = R"(
        CREATE TABLE IF NOT EXISTS files (
            id INTEGER PRIMARY KEY AUTOINCREMENT, path TEXT UNIQUE NOT NULL COLLATE NOCASE,
            size BIGINT NOT NULL, mtime BIGINT NOT NULL, xxhash32 BIGINT NOT NULL,
            sha256 BLOB(32) NOT NULL, last_scan INTEGER DEFAULT (strftime('%s', 'now'))
        );
        CREATE INDEX IF NOT EXISTS idx_files_path ON files(path);
        CREATE INDEX IF NOT EXISTS idx_files_size ON files(size);
    )";

    sqlite3* db_ = nullptr;
    std::wstring db_path_;
};