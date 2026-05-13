#include "DatabaseManager.h"

static const char* SCHEMA = R"(
CREATE TABLE IF NOT EXISTS files (
    id INTEGER PRIMARY KEY AUTOINCREMENT, path TEXT UNIQUE NOT NULL COLLATE NOCASE,
    size BIGINT NOT NULL, mtime BIGINT NOT NULL, xxhash32 BIGINT NOT NULL,
    sha256 BLOB(32) NOT NULL, last_scan INTEGER DEFAULT (strftime('%s', 'now'))
);
CREATE INDEX IF NOT EXISTS idx_files_path ON files(path);
CREATE INDEX IF NOT EXISTS idx_files_size ON files(size);
CREATE INDEX IF NOT EXISTS idx_files_xxhash ON files(xxhash32);

CREATE TABLE IF NOT EXISTS scan_sessions (
    id INTEGER PRIMARY KEY AUTOINCREMENT, path_hash BIGINT DEFAULT 0,
    scan_path TEXT,
    created_at BIGINT DEFAULT (strftime('%s', 'now')),
    file_count INT, duplicate_count INT, strategy_flags INT
);

CREATE TABLE IF NOT EXISTS action_history (
    id INTEGER PRIMARY KEY AUTOINCREMENT, session_id INT REFERENCES scan_sessions(id),
    file_path TEXT NOT NULL, action_type TEXT, old_value TEXT, new_value TEXT,
    performed_at BIGINT DEFAULT (strftime('%s', 'now'))
);
CREATE INDEX IF NOT EXISTS idx_action_session ON action_history(session_id);
)";

DatabaseManager::DatabaseManager(const std::wstring& db_path) : db_path_(db_path) {}
DatabaseManager::~DatabaseManager() { if (db_) sqlite3_close(db_); }

bool DatabaseManager::init() {
    std::string utf8_db_path = PathUtils::wide_to_utf8(db_path_);
    int rc = sqlite3_open_v2(utf8_db_path.c_str(), &db_, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
    if (rc != SQLITE_OK) return false;
    const char* wal_sql = "PRAGMA journal_mode=WAL;";
    return sqlite3_exec(db_, wal_sql, nullptr, nullptr, nullptr) == SQLITE_OK
        && sqlite3_exec(db_, SCHEMA, nullptr, nullptr, nullptr) == SQLITE_OK;
}

bool DatabaseManager::upsert_file(const FileInfo& info, long long last_scan_seconds) {
    const char* sql = "UPDATE files SET xxhash32=?, sha256=?, mtime=?, last_scan=? WHERE path=?";
    sqlite3_stmt* stmt; int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;

    std::vector<uint8_t> sha_blob(info.sha256.begin(), info.sha256.end());
    const std::string utf_path = PathUtils::wide_to_utf8(info.path);
    sqlite3_bind_int64(stmt, 1, static_cast<int64_t>(info.xxhash));
    sqlite3_bind_blob(stmt, 2, sha_blob.data(), static_cast<int>(sha_blob.size()), SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 3, info.mtime);
    sqlite3_bind_int64(stmt, 4, last_scan_seconds);
    sqlite3_bind_text(stmt, 5, utf_path.c_str(), static_cast<int>(utf_path.size()), SQLITE_TRANSIENT);

    int changes = (sqlite3_step(stmt) == SQLITE_DONE) ? sqlite3_changes(db_) : 0;
    sqlite3_finalize(stmt);
    if (changes > 0) return true;

    const char* ins_sql = "INSERT OR IGNORE INTO files (path, size, mtime, xxhash32, sha256, last_scan) VALUES (?, ?, ?, ?, ?, ?)";
    rc = sqlite3_prepare_v2(db_, ins_sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, utf_path.c_str(), static_cast<int>(utf_path.size()), SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, info.size);
    sqlite3_bind_int64(stmt, 3, info.mtime);
    sqlite3_bind_int64(stmt, 4, static_cast<int64_t>(info.xxhash));
    sqlite3_bind_blob(stmt, 5, sha_blob.data(), static_cast<int>(sha_blob.size()), SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 6, last_scan_seconds);

    int ins_changes = (sqlite3_step(stmt) == SQLITE_DONE) ? sqlite3_changes(db_) : 0;
    sqlite3_finalize(stmt);
    return ins_changes > 0;
}

std::vector<FileInfo> DatabaseManager::get_cached_files() const {
    std::vector<FileInfo> results;
    const char* sql = "SELECT path, size, mtime, xxhash32, sha256 FROM files ORDER BY path COLLATE NOCASE";
    sqlite3_stmt* stmt; int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return results;

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        FileInfo fi{};
        const unsigned char* path_raw = sqlite3_column_text(stmt, 0);
        if (path_raw) fi.path = PathUtils::utf8_to_wide(reinterpret_cast<const char*>(path_raw));
        fi.size = static_cast<uint64_t>(sqlite3_column_int64(stmt, 1));
        fi.mtime = sqlite3_column_int64(stmt, 2);
        fi.xxhash = static_cast<XxHash32>(sqlite3_column_int64(stmt, 3));
        const unsigned char* sha_raw = static_cast<const unsigned char*>(sqlite3_column_blob(stmt, 4));
        int sha_len = sqlite3_column_bytes(stmt, 4);
        if (sha_raw && sha_len == 32) std::copy(sha_raw, sha_raw + 32, fi.sha256.begin());
        results.push_back(std::move(fi));
    }
    sqlite3_finalize(stmt);
    return results;
}

bool DatabaseManager::remove_deleted_files(const std::vector<std::wstring>& current_paths) {
    // If no paths provided, keep all existing records rather than deleting everything.
    if (current_paths.empty()) return true;
    std::string placeholders;
    placeholders.reserve(current_paths.size() * 2);
    for (size_t i = 0; i < current_paths.size(); ++i) {
        if (i > 0) placeholders += ',';
        placeholders += '?';
    }
    std::string sql = "DELETE FROM files WHERE path NOT IN (" + placeholders + ")";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return false;

    for (size_t i = 0; i < current_paths.size(); ++i) {
        std::string utf8_path = PathUtils::wide_to_utf8(current_paths[i]);
        sqlite3_bind_text(stmt, static_cast<int>(i + 1), utf8_path.c_str(),
                          static_cast<int>(utf8_path.size()), SQLITE_TRANSIENT);
    }

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

bool DatabaseManager::save_session(int64_t path_hash, const std::string& scan_path, int file_count, int duplicate_count, uint32_t strategy_flags) {
    const char* sql = "INSERT INTO scan_sessions (path_hash, scan_path, created_at, file_count, duplicate_count, strategy_flags) VALUES (?, ?, strftime('%s', 'now'), ?, ?, ?)";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int64(stmt, 1, path_hash);
    sqlite3_bind_text(stmt, 2, scan_path.c_str(), static_cast<int>(scan_path.size()), SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 3, file_count);
    sqlite3_bind_int64(stmt, 4, duplicate_count);
    sqlite3_bind_int64(stmt, 5, strategy_flags);

    int changes = (sqlite3_step(stmt) == SQLITE_DONE) ? sqlite3_changes(db_) : 0;
    sqlite3_finalize(stmt);
    return changes > 0;
}

int DatabaseManager::get_last_session_id() const {
    const char* sql = "SELECT MAX(id) FROM scan_sessions";
    sqlite3_stmt* stmt; int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return 0;
    int result = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) result = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    return result;
}

bool DatabaseManager::record_action(int session_id, const std::wstring& file_path,
                                     const std::string& action_type,
                                     const std::string& old_value,
                                     const std::string& new_value) {
    const char* sql = "INSERT INTO action_history (session_id, file_path, action_type, old_value, new_value, performed_at) VALUES (?, ?, ?, ?, ?, strftime('%s', 'now'))";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    std::string path_str = PathUtils::wide_to_utf8(file_path);
    sqlite3_bind_int64(stmt, 1, session_id);
    sqlite3_bind_text(stmt, 2, path_str.c_str(), static_cast<int>(path_str.size()), SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, action_type.empty() ? nullptr : action_type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, old_value.empty() ? nullptr : old_value.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, new_value.empty() ? nullptr : new_value.c_str(), -1, SQLITE_TRANSIENT);

    int changes = (sqlite3_step(stmt) == SQLITE_DONE) ? sqlite3_changes(db_) : 0;
    sqlite3_finalize(stmt);
    return changes > 0;
}