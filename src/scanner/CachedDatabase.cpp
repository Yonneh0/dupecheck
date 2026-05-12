#include "CachedDatabase.h"
#include "../hashing/HashEngine.h"

bool CachedDatabase::init() {
    int rc = sqlite3_open_v2(db_path_.c_str(), &db_, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
    if (rc != SQLITE_OK) return false;
    const char* wal_sql = "PRAGMA journal_mode=WAL;";
    return sqlite3_exec(db_, wal_sql, nullptr, nullptr, nullptr) == SQLITE_OK &&
           sqlite3_exec(db_, SCHEMA, nullptr, nullptr, nullptr) == SQLITE_OK;
}

std::vector<FileInfo> CachedDatabase::get_cached_files() const {
    std::vector<FileInfo> results;
    const char* sql = "SELECT path, size, mtime, xxhash32, sha256 FROM files ORDER BY size DESC";
    sqlite3_stmt* stmt; int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
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
        if (sha_raw && sha_len == 32) std::copy(sha_raw, sha_raw + 32, fi.sha256.begin());
        results.push_back(std::move(fi));
    }
    sqlite3_finalize(stmt);
    return results;
}

bool CachedDatabase::upsert_file(const FileInfo& info, long long last_scan_seconds) {
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

    rc = sqlite3_step(stmt); int changes = sqlite3_changes(db_); sqlite3_finalize(stmt);
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

    rc = sqlite3_step(stmt); int ins_changes = sqlite3_changes(db_); sqlite3_finalize(stmt);
    return ins_changes > 0;
}

bool CachedDatabase::remove_deleted_files(const std::vector<std::wstring>& current_paths) {
    if (current_paths.empty()) {
        sqlite3_exec(db_, "DELETE FROM files", nullptr, nullptr, nullptr);
        return true;
    }

    std::string where = "WHERE path NOT IN (";
    bool first = true;
    for (const auto& p : current_paths) {
        std::string utf8_path = PathUtils::wide_to_utf8(p);
        std::string escaped;
        escaped.reserve(utf8_path.size() * 2 + 1);
        for (char c : utf8_path) {
            if (c == '\'') escaped += "''";
            else escaped += c;
        }
        where += first ? "'" + escaped + "'" : ", '" + escaped + "'";
        first = false;
    }
    where += ")";

    std::string sql = "DELETE FROM files " + where;
    return sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, nullptr) == SQLITE_OK;
}