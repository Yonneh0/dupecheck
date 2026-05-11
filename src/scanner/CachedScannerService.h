#pragma once
#include <string>
#include "../core/FileInfo.h"
#include "../database/DatabaseManager.h"

class CachedScannerService {
public:
    explicit CachedScannerService(const std::wstring& db_path);
    
    bool init();
    
    std::vector<FileInfo> scan(const wchar_t* path);

private:
    DatabaseManager db_;
};
