#pragma once
#include <vector>
#include "../core/FileInfo.h"
#include "CachedDatabase.h"

// Legacy file scanner — kept for compatibility.
class FileScanner {
public:
    explicit FileScanner(const std::wstring& db_path);
    bool init();
    std::vector<FileInfo> scan(const wchar_t* path);

private:
    CachedDatabase cache_;
};