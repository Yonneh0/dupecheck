#pragma once
#include <vector>
#include <array>
#include <unordered_map>
#include "../core/FileInfo.h"

/// Group files by SHA256; each group with >1 file is a duplicate set.
inline std::vector<DuplicateGroup> exact_match(const std::vector<FileInfo>& files) {
    std::unordered_map<std::array<uint8_t, 32>, std::vector<FileInfo>> sha_groups;
    for (const auto& f : files) {
        sha_groups[f.sha256].push_back(f);
    }

    std::vector<DuplicateGroup> groups;
    groups.reserve(sha_groups.size());
    for (auto& [sha, group] : sha_groups) {
        if (group.size() > 1) {
            DuplicateGroup dg{std::move(group), Strategy::ExactMatch};
            dg.label = "Exact Match (" + std::to_string(dg.files.size()) + " files)";
            groups.push_back(std::move(dg));
        }
    }
    return groups;
}