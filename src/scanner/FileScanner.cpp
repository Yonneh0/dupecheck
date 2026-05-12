#include "FileScanner.h"

// Inherits scan() and init() from CachedScannerService directly.
bool FileScanner::init() { return cache_.init(); }
std::vector<FileInfo> FileScanner::scan(const wchar_t* path) {
    return static_cast<CachedScannerService*>(this)->CachedScannerService::scan(path);
}