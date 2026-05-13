#pragma once
#include <vector>
#include <array>
#include <unordered_map>
#include "../core/FileInfo.h"

inline std::vector<DuplicateGroup> exact_match(const std::vector<FileInfo>& files) {
    std::unordered_map<Sha256, std::vector<FileInfo>, Sha256Hash> sha_groups;
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