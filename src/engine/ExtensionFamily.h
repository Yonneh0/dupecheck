#pragma once
#include <vector>
#include "../core/FileInfo.h"
#include "../utils/ExtensionFamilyMap.h"

inline std::vector<DuplicateGroup> extension_family(const std::vector<FileInfo>& files) {
    std::unordered_map<Sha256, std::vector<FileInfo>, Sha256Hash> sha_groups;
    for (const auto& f : files) sha_groups[f.sha256].push_back(f);

    std::vector<DuplicateGroup> groups;
    for (auto& [sha, group] : sha_groups) {
        if (group.size() < 2) continue;

        bool found_family_match = false;
        for (size_t i = 0; i < group.size() && !found_family_match; ++i) {
            for (size_t j = i + 1; j < group.size(); ++j) {
                if (PathUtils::get_extension(group[i].path) != PathUtils::get_extension(group[j].path)) {
                    if (ExtensionFamilyMap::is_same_family(PathUtils::get_extension(group[i].path),
                                                           PathUtils::get_extension(group[j].path)))
                        found_family_match = true;
                }
            }
        }

        if (found_family_match) {
            DuplicateGroup dg{std::move(group), Strategy::ExtensionFamily};
            dg.label = "Extension Family (" + std::to_string(dg.files.size()) + " files)";
            groups.push_back(std::move(dg));
        }
    }
    return groups;
}