#pragma once
#include <string>
#include <vector>
#include <sqlite3.h>
#include "../core/FileInfo.h"

// SQLite-backed persistence layer for file metadata, scan sessions, and action history.
class DatabaseManager {
public:
    explicit DatabaseManager(const std::wstring& db_path);
    ~DatabaseManager();
    bool init();
    // Insert or update a file's cached metadata. Returns true if the file was stored (updated or newly inserted).
    bool upsert_file(const FileInfo& info, long long last_scan_seconds);
    // Retrieve all cached files.
    std::vector<FileInfo> get_cached_files() const;
    // Remove entries for files that no longer exist on disk.
    bool remove_deleted_files(const std::vector<std::wstring>& current_paths);
    // Persist a scan session record.
    bool save_session(int64_t path_hash, int file_count, int duplicate_count, uint32_t strategy_flags);
    int get_last_session_id() const;
    // Log an action taken on a file during the given session.
    bool record_action(int session_id, const std::wstring& file_path,
                       const std::string& action_type,
                       const std::string& old_value,
                       const std::string& new_value);

private:
    sqlite3* db_ = nullptr;
    std::wstring db_path_;
};