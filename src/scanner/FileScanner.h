#pragma once
#include <string>
#include "../core/FileInfo.h"
#include "CachedScannerService.h"

// Legacy scanner — wraps CachedScannerService with composition instead of inheritance.
class FileScanner {
public:
    explicit FileScanner(const std::wstring& db_path) : cache_(db_path) {}
    bool init() { return cache_.init(); }
    // Scan the given directory, merging new entries with cached metadata.
    std::vector<FileInfo> scan(const wchar_t* path) { return cache_.scan(path); }

private:
    CachedScannerService cache_;
};