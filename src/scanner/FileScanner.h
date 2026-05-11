#pragma once
#include "../core/FileInfo.h"
#include <vector>
#include <functional>

class FileScanner {
public:
    using ProgressCallback = std::function<void(uint64_t total, uint64_t processed)>;
    explicit FileScanner(ProgressCallback progress = nullptr) : progress_(std::move(progress)) {}
    std::vector<FileInfo> scan(const wchar_t* path);
private:
    ProgressCallback progress_;
};