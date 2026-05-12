#pragma once
#include <string>
#include <vector>
#include <sqlite3.h>
#include "../core/FileInfo.h"

/// SQLite-backed persistence layer for file metadata, scan sessions, and action history.
/// Uses WAL mode for better concurrent access performance.
class DatabaseManager {
public:
    explicit DatabaseManager(const std::wstring& db_path);
    ~DatabaseManager();

    /// Initialize the database (creates tables if they don't exist). Returns false on failure.
    bool init();

    /// Insert or update a file's cached metadata. Returns true if stored or updated.
    bool upsert_file(const FileInfo& info, long long last_scan_seconds);

    /// Retrieve all cached files sorted by size descending.
    std::vector<FileInfo> get_cached_files() const;

    /// Remove entries for files that no longer exist on disk. If current_paths is empty, clears all entries.
    bool remove_deleted_files(const std::vector<std::wstring>& current_paths);

    /// Persist a scan session record. Returns true if saved.
    bool save_session(int64_t path_hash, int file_count, int duplicate_count, uint32_t strategy_flags);

    /// Get the ID of the last inserted session (0 if none).
    int get_last_session_id() const;

    /// Log an action taken on a file during the given session. Returns true if saved.
    bool record_action(int session_id, const std::wstring& file_path,
                       const std::string& action_type,
                       const std::string& old_value,
                       const std::string& new_value);

private:
    sqlite3* db_ = nullptr;
    std::wstring db_path_;
};