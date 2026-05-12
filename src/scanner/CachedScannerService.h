#pragma once
#include <string>
#include <vector>
#include "../core/FileInfo.h"
#include "../database/DatabaseManager.h"

/// SQLite-backed scanner that reuses cached metadata for unchanged files.
class CachedScannerService {
public:
    /// Initialize the database connection (must be called before scan()).
    bool init();
    /// Scan the given directory, merging new entries with cached metadata.
    std::vector<FileInfo> scan(const wchar_t* path);

private:
    DatabaseManager manager_{L""};  // Default-constructed; db_path set via init().
};
