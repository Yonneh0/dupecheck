#pragma once
#include <string>
#include "../core/FileInfo.h"
#include "../database/DatabaseManager.h"

// Cached scanner service that persists scan results in SQLite and supports incremental updates.
class CachedScannerService {
public:
    explicit CachedScannerService(const std::wstring& db_path);
    
    // Initialize the database connection.
    bool init();
    
    // Scan a directory, using cached data when possible (incremental).
    std::vector<FileInfo> scan(const wchar_t* path);

private:
    DatabaseManager db_;
};