#pragma once
#include "../core/FileInfo.h"
#include "CachedScannerService.h"

// Legacy scanner — inherits from CachedScannerService for compatibility.
class FileScanner : public CachedScannerService {
public:
    explicit FileScanner(const std::wstring& db_path) : CachedScannerService(db_path) {}
    bool init();
    std::vector<FileInfo> scan(const wchar_t* path);
};