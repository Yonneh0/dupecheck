#pragma once
#include <vector>
#include "../core/FileInfo.h"
#include "DuplicateEngine.h"

/// Strategy 5a (ExactMatch): Group files by SHA256 hash; any group with more than one file is a duplicate set.
inline std::vector<DuplicateGroup> exact_match(const std::vector<FileInfo>& files) {
    std::unordered_map<std::array<uint8_t, 32>, std::vector<FileInfo>> sha_groups;

    // Group all files by their SHA256 hash.
    for (auto& f : files) {
        sha_groups[f.sha256].push_back(std::move(f));
    }

    std::vector<DuplicateGroup> groups;
    for (auto& [sha, group] : sha_groups) {
        if (group.size() > 1) {
            DuplicateGroup dg;
            dg.files = std::move(group);
            dg.strategy = Strategy::ExactMatch;
            dg.label = "Exact Match (" + std::to_string(dg.files.size()) + " files)";
            groups.push_back(std::move(dg));
        }
    }

    return groups;
}