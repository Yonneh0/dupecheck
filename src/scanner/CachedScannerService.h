#pragma once
#include <string>
#include "../core/FileInfo.h"
#include "../database/DatabaseManager.h"

// SQLite-backed scanner that reuses cached metadata for unchanged files.
class CachedScannerService {
public:
    explicit CachedScannerService(const std::wstring& db_path, const std::wstring& cache_db_path = L"");
    bool init();
    // Scan the given directory, merging new entries with cached metadata.
    std::vector<FileInfo> scan(const wchar_t* path);

private:
    DatabaseManager manager_;
};