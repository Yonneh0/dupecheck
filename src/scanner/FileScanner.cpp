#include "FileScanner.h"
#include "../hashing/HashEngine.h"
#include <algorithm>

std::vector<FileInfo> FileScanner::scan(const wchar_t* path) {
    std::vector<PathUtils::FileEntry> entries;
    PathUtils::enumerate_files(path, entries);

    size_t total = entries.size();
    if (progress_) progress_(total, 0);

    // Initialize results with file metadata.
    std::vector<FileInfo> results(entries.size());
    for (size_t i = 0; i < entries.size(); ++i) {
        auto& entry = entries[i];
        results[i].path = entry.path;
        results[i].size = entry.size;
        results[i].mtime = entry.mtime;
    }

    // Compute hashes in parallel.
    std::vector<std::future<void>> futures;
    futures.reserve(entries.size());

    for (size_t i = 0; i < entries.size(); ++i) {
        const auto& entry = entries[i];
        futures.push_back(std::async(std::launch::async, [this, &entry, &results, i]() -> FileInfo {
            HashResult hr = HashEngine::compute(entry.path.c_str());
            return FileInfo{entry.path, entry.size, entry.mtime, hr.xxhash, hr.sha256};
        }));
    }

    // Collect results and update progress.
    for (size_t i = 0; i < entries.size(); ++i) {
        results[i] = futures[i].get();
        if (progress_) progress_(total, i + 1);
    }

    return results;
}