#pragma once
#include <vector>
#include "../core/FileInfo.h"
#include "../utils/ExtensionFamilyMap.h"
#include "DuplicateEngine.h"

inline std::vector<DuplicateGroup> extension_family(const std::vector<FileInfo>& files) {
    std::unordered_map<std::array<uint8_t, 32>, std::vector<FileInfo>> sha_groups;
    for (auto& f : files) sha_groups[f.sha256].push_back(f);

    std::vector<DuplicateGroup> groups;
    for (const auto& [sha, group] : sha_groups) {
        if (group.size() < 2) continue;

        bool found_family_match = false;
        for (size_t i = 0; i < group.size() && !found_family_match; ++i) {
            for (size_t j = i + 1; j < group.size(); ++j) {
                auto ext_i = PathUtils::get_extension(group[i].path);
                auto ext_j = PathUtils::get_extension(group[j].path);
                if (ext_i != ext_j && ExtensionFamilyMap::is_same_family(ext_i, ext_j)) {
                    found_family_match = true;
                    break;
                }
            }
        }

        if (found_family_match) {
            DuplicateGroup dg;
            for (auto& f : group) dg.files.push_back(std::move(f));
            dg.strategy = Strategy::ExtensionFamily;
            dg.label = "Extension Family (" + std::to_string(dg.files.size()) + " files)";
            groups.push_back(std::move(dg));
        }
    }
    return groups;
}