#pragma once
#include "../core/FileInfo.h"
#include "../hashing/HashEngine.h"
#include <vector>
#include <functional>

class FileScanner {
public:
    using ProgressCallback = std::function<void(uint64_t total, uint64_t processed)>;
    
    explicit FileScanner(ProgressCallback progress = nullptr);
    
    // Scan a directory and return all files with computed hashes.
    std::vector<FileInfo> scan(const wchar_t* path);

private:
    ProgressCallback progress_;
};