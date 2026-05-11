#pragma once
#include <vector>
#include "../core/FileInfo.h"
#include "../utils/Levenshtein.h"
#include "DuplicateEngine.h"

inline std::vector<DuplicateGroup> name_variant(const std::vector<FileInfo>& files, int threshold) {
    std::unordered_map<std::array<uint8_t, 32>, std::vector<FileInfo>> sha_groups;
    for (auto& f : files) sha_groups[f.sha256].push_back(std::move(f));

    std::vector<DuplicateGroup> groups;
    for (auto& [sha, group] : sha_groups) {
        if (group.size() < 2) continue;
        bool found_variant = false;
        for (size_t i = 0; i < group.size(); ++i)
            for (size_t j = i + 1; j < group.size(); ++j)
                if (levenshtein_distance(PathUtils::get_name_without_ext(group[i].path), PathUtils::get_name_without_ext(group[j].path)) <= threshold) found_variant = true;

        if (found_variant && group.size() > 1) {
            DuplicateGroup dg; dg.files = std::move(group);
            dg.strategy = Strategy::NameVariant;
            dg.label = "Name Variants (" + std::to_string(dg.files.size()) + " files)";
            groups.push_back(std::move(dg));
        }
    }
    return groups;
}