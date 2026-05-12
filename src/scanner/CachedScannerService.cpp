#include "CachedScannerService.h"
#include "../hashing/HashEngine.h"

// SQLite-backed scanner that reuses cached metadata for unchanged files.
class CachedDatabase {
public:
    explicit CachedDatabase(const std::wstring& db_path) : db_path_(db_path) {}
    bool init() {
        int rc = sqlite3_open_v2(db_path_.c_str(), &db_, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
        if (rc != SQLITE_OK) return false;
        const char* wal_sql = "PRAGMA journal_mode=WAL;";
        return sqlite3_exec(db_, wal_sql, nullptr, nullptr, nullptr) == SQLITE_OK &&
               sqlite3_exec(db_, SCHEMA, nullptr, nullptr, nullptr) == SQLITE_OK;
    }

    std::vector<FileInfo> get_cached_files() const {
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
        sqlite3_finalize(stmt); return results;
    }

    bool upsert_file(const FileInfo& info, long long last_scan_seconds) {
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

    bool remove_deleted_files(const std::vector<std::wstring>& current_paths) {
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

CachedScannerService::CachedScannerService(const std::wstring& db_path) : db_path_(db_path) {}

bool CachedScannerService::init() { return cache_.init(); }

std::vector<FileInfo> CachedScannerService::scan(const wchar_t* path) {
    std::vector<PathUtils::FileInfo> current_entries;
    PathUtils::enumerate_files(path, current_entries);

    auto cached = cache_.get_cached_files();

    // Remove files that no longer exist.
    std::vector<std::wstring> current_paths(current_entries.size());
    for (size_t i = 0; i < current_entries.size(); ++i) {
        current_paths[i] = current_entries[i].path;
    }
    cache_.remove_deleted_files(current_paths);

    // Merge cached and new entries.
    std::vector<FileInfo> results;
    for (auto& entry : current_entries) {
        bool found = false;
        for (const auto& cf : cached) {
            if (cf.path == entry.path && cf.size == entry.size && cf.mtime == entry.mtime) {
                results.push_back(cf);
                found = true;
                break;
            }
        }
        if (!found) {
            HashResult hr = HashEngine::compute(entry.path.c_str());
            FileInfo fi{entry.path, entry.size, entry.mtime, hr.xxhash, hr.sha256};
            cache_.upsert_file(fi, static_cast<long long>(std::time(nullptr)));
            results.push_back(std::move(fi));
        }
    }
    return results;
}