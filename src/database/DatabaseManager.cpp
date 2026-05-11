#include "DatabaseManager.h"
#include <fstream>
#include <cstdio>

static const char* SCHEMA = R"(
CREATE TABLE IF NOT EXISTS files (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    path TEXT UNIQUE NOT NULL COLLATE NOCASE,
    size BIGINT NOT NULL,
    mtime BLOB(8) NOT NULL,
    xxhash32 INTEGER NOT NULL,
    sha256 BLOB(32) NOT NULL,
    last_scan INTEGER NOT NULL DEFAULT (strftime('%s', 'now'))
);

CREATE INDEX IF NOT EXISTS idx_files_path ON files(path);
CREATE INDEX IF NOT EXISTS idx_files_size ON files(size);
CREATE INDEX IF NOT EXISTS idx_files_xxhash ON files(xxhash32);

/* Note: directories table was removed — FolderCopy now computes tree hashes inline. */

CREATE TABLE IF NOT EXISTS scan_sessions (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    scan_path TEXT NOT NULL,
    path_hash BIGINT NOT NULL DEFAULT 0,
    created_at BIGINT NOT NULL DEFAULT (strftime('%s', 'now')),
    file_count INTEGER NOT NULL,
    duplicate_count INTEGER NOT NULL,
    strategy_flags INTEGER NOT NULL
);

CREATE TABLE IF NOT EXISTS action_history (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    session_id INTEGER REFERENCES scan_sessions(id),
    file_path TEXT NOT NULL,
    action_type TEXT NOT NULL,
    old_value TEXT,
    new_value TEXT,
    performed_at BIGINT NOT NULL DEFAULT (strftime('%s', 'now'))
);

CREATE INDEX IF NOT EXISTS idx_action_session ON action_history(session_id);
)";

DatabaseManager::DatabaseManager(const std::wstring& db_path) : db_path_(db_path) {}
DatabaseManager::~DatabaseManager() {
    if (db_) sqlite3_close(db_);
}

bool DatabaseManager::init() {
    // Create parent directory if needed.
    auto dir = db_path_.substr(0, db_path_.find_last_of(L"\\"));
    
    int rc = sqlite3_open_v2(db_path_.c_str(), &db_, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
    if (rc != SQLITE_OK) return false;

    // Enable WAL mode.
    const char* wal_sql = "PRAGMA journal_mode=WAL;";
    rc = sqlite3_exec(db_, wal_sql, nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK) return false;

    rc = sqlite3_exec(db_, SCHEMA, nullptr, nullptr, nullptr);
    return rc == SQLITE_OK;
}

bool DatabaseManager::upsert_file(const FileInfo& info, long long last_scan_seconds) {
    const char* sql = "UPDATE files SET xxhash32=?, sha256=?, mtime=?, last_scan=? WHERE path=?";
    sqlite3_stmt* stmt;

    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;

    std::vector<uint8_t> sha_blob(info.sha256.begin(), info.sha256.end());

    sqlite3_bind_int64(stmt, 1, static_cast<int64_t>(info.xxhash));
    sqlite3_bind_blob(stmt, 2, sha_blob.data(), static_cast<int>(sha_blob.size()), SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 3, info.mtime);
    sqlite3_bind_int64(stmt, 4, last_scan_seconds);
    std::string utf_path_update = PathUtils::wide_to_utf8(info.path);
    sqlite3_bind_text(stmt, 5, utf_path_update.c_str(), static_cast<int>(utf_path_update.size()), SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    int changes = sqlite3_changes(db_);
    sqlite3_finalize(stmt);

    if (changes > 0) return true;

    const char* ins_sql = "INSERT OR IGNORE INTO files (path, size, mtime, xxhash32, sha256, last_scan) VALUES (?, ?, ?, ?, ?, ?)";
    rc = sqlite3_prepare_v2(db_, ins_sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;

    std::string utf_path_insert = PathUtils::wide_to_utf8(info.path);
    sqlite3_bind_text(stmt, 1, utf_path_insert.c_str(), static_cast<int>(utf_path_insert.size()), SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, info.size);
    sqlite3_bind_int64(stmt, 3, info.mtime);
    sqlite3_bind_int64(stmt, 4, static_cast<int64_t>(info.xxhash));
    sqlite3_bind_blob(stmt, 5, sha_blob.data(), static_cast<int>(sha_blob.size()), SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 6, last_scan_seconds);

    rc = sqlite3_step(stmt);
    int ins_changes = sqlite3_changes(db_);
    sqlite3_finalize(stmt);

    return ins_changes > 0;
}

std::vector<FileInfo> DatabaseManager::get_cached_files() const {
    std::vector<FileInfo> results;
    const char* sql = "SELECT path, size, mtime, xxhash32, sha256 FROM files ORDER BY size DESC";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return results;

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        FileInfo fi;

        const unsigned char* path_raw = sqlite3_column_text(stmt, 0);
        if (path_raw) fi.path = PathUtils::utf8_to_wide(reinterpret_cast<const char*>(path_raw));

        fi.size = static_cast<uint64_t>(sqlite3_column_int64(stmt, 1));
        fi.mtime = sqlite3_column_int64(stmt, 2);
        fi.xxhash = static_cast<XxHash32>(sqlite3_column_int64(stmt, 3));

        const unsigned char* sha_raw = sqlite3_column_blob(stmt, 4);
        int sha_len = sqlite3_column_bytes(stmt, 4);
        if (sha_raw && sha_len == 32) {
            std::copy(sha_raw, sha_raw + 32, fi.sha256.begin());
        }

        results.push_back(std::move(fi));
    }

    sqlite3_finalize(stmt);
    return results;
}

bool DatabaseManager::remove_deleted_files(const std::vector<std::wstring>& current_paths) {
    sqlite3_exec(db_, "BEGIN TRANSACTION", nullptr, nullptr, nullptr);

    const char* del_sql = "DELETE FROM files";
    sqlite3_exec(db_, del_sql, nullptr, nullptr, nullptr);

    if (current_paths.empty()) {
        sqlite3_exec(db_, "COMMIT", nullptr, nullptr, nullptr);
        return true;
    }

    const char* ins_sql = "INSERT OR IGNORE INTO files (path) VALUES (?)";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, ins_sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        sqlite3_exec(db_, "ROLLBACK", nullptr, nullptr, nullptr);
        return false;
    }

    for (const auto& p : current_paths) {
        std::string utf8_path = PathUtils::wide_to_utf8(p);
        sqlite3_bind_text(stmt, 1, utf8_path.c_str(), static_cast<int>(utf8_path.size()), SQLITE_STATIC);
        rc = sqlite3_step(stmt);
        sqlite3_reset(stmt);
    }

    sqlite3_finalize(stmt);
    sqlite3_exec(db_, "COMMIT", nullptr, nullptr, nullptr);

    return true;
}

bool DatabaseManager::save_session(int64_t path_hash, int file_count, int duplicate_count, uint32_t strategy_flags) {
    const char* sql = "INSERT INTO scan_sessions (path_hash, created_at, file_count, duplicate_count, strategy_flags) VALUES (?, strftime('%s', 'now'), ?, ?, ?)";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;

    sqlite3_bind_int64(stmt, 1, path_hash);
    sqlite3_bind_int(stmt, 2, file_count);
    sqlite3_bind_int(stmt, 3, duplicate_count);
    sqlite3_bind_int(stmt, 4, strategy_flags);

    rc = sqlite3_step(stmt);
    int changes = sqlite3_changes(db_);
    sqlite3_finalize(stmt);

    return changes > 0;
}

int DatabaseManager::get_last_session_id() const {
    const char* sql = "SELECT MAX(id) FROM scan_sessions";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return 0;

    int result = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        result = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return result;
}

bool DatabaseManager::record_action(int session_id, const std::wstring& file_path,
                                     const std::string& action_type,
                                     const std::string& old_value,
                                     const std::string& new_value) {
    const char* sql = "INSERT INTO action_history (session_id, file_path, action_type, old_value, new_value, performed_at) VALUES (?, ?, ?, ?, strftime('%s', 'now'))";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;

    std::string path_str = PathUtils::wide_to_utf8(file_path);
    sqlite3_bind_int(stmt, 1, session_id);
    sqlite3_bind_text(stmt, 2, path_str.c_str(), static_cast<int>(path_str.size()), SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, action_type.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, old_value.empty() ? nullptr : old_value.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, new_value.empty() ? nullptr : new_value.c_str(), -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    int changes = sqlite3_changes(db_);
    sqlite3_finalize(stmt);

    return changes > 0;
}