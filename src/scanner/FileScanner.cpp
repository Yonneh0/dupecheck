#include "FileScanner.h"

FileScanner::FileScanner(ProgressCallback progress) : progress_(std::move(progress)) {}

std::vector<FileInfo> FileScanner::scan(const wchar_t* path) {
    std::vector<PathUtils::FileEntry> entries;
    PathUtils::enumerate_files(path, entries);

    if (progress_) progress_(entries.size(), 0);

    std::vector<FileInfo> results(entries.size());
    
    for (size_t i = 0; i < entries.size(); ++i) {
        auto& entry = entries[i];
        results[i].path = entry.path;
        results[i].size = entry.size;
        results[i].mtime = entry.mtime;
    }

    std::vector<std::wstring> paths(entries.size());
    for (size_t i = 0; i < entries.size(); ++i) {
        paths[i] = entries[i].path;
    }

    HashEngine::compute_batch(paths, results);

    return results;
}
