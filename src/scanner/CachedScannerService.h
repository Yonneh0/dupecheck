#pragma once
#include <string>
#include <vector>
#include "../core/FileInfo.h"
#include "../database/DatabaseManager.h"

/// SQLite-backed scanner that reuses cached metadata for unchanged files.
/// On scan, compares file size and mtime against cache; only re-hashes changed files.
class CachedScannerService {
public:
    /// Initialize the database connection (must be called before scan()). Returns false on failure.
    bool init();

    /// Scan the given directory, merging new entries with cached metadata. Returns all file info.
    std::vector<FileInfo> scan(const wchar_t* path);

private:
    DatabaseManager manager_{L""};  // Default-constructed; db_path set via init().
};