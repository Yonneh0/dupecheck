#pragma once
#include <string>
#include <vector>
#include <sqlite3.h>
#include "../core/FileInfo.h"

class DatabaseManager {
public:
    explicit DatabaseManager(const std::wstring& db_path);
    ~DatabaseManager();

    // Initialize the database schema.
    bool init();

    // Insert or update a file entry (upsert).
    bool upsert_file(const FileInfo& info, long long last_scan_seconds);

    // Get all files from cache matching paths.
    std::vector<FileInfo> get_cached_files() const;

    // Delete files that no longer exist on disk.
    bool remove_deleted_files(const std::vector<std::wstring>& current_paths);

    // Save a scan session record.
    bool save_session(int64_t path_hash, int file_count, int duplicate_count, uint32_t strategy_flags);

    // Get the last session ID.
    int get_last_session_id() const;

    // Record an action for undo support.
    bool record_action(int session_id, const std::wstring& file_path, 
                       const std::string& action_type, 
                       const std::string& old_value, 
                       const std::string& new_value);

private:
    sqlite3* db_ = nullptr;
    std::wstring db_path_;
};