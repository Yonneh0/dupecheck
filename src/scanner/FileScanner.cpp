#include "FileScanner.h"

FileScanner::FileScanner(ProgressCallback progress) : progress_(std::move(progress)) {}

std::vector<FileInfo> FileScanner::scan(const wchar_t* path) {
    std::vector<PathUtils::FileEntry> entries;
    PathUtils::enumerate_files(path, entries);

    if (progress_) progress_(entries.size(), 0);

    // Use the HashEngine thread pool for better concurrency control.
    std::vector<FileInfo> results(entries.size());
    uint64_t idx = 0;

    for (size_t i = 0; i < entries.size(); ++i) {
        auto& entry = entries[i];
        results[i].path = entry.path;
        results[i].size = entry.size;
        results[i].mtime = entry.mtime;

        HashResult hr = HashEngine::compute(entry.path.c_str());
        results[i].xxhash = hr.xxhash;
        std::copy(std::begin(hr.sha256), std::end(hr.sha256), std::begin(results[i].sha256));

        idx++;
        if (progress_ && (i % 100 == 99 || i == entries.size() - 1)) {
            progress_(entries.size(), idx);
        }
    }

    return results;
}
