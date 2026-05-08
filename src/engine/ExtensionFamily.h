#pragma once
#include <vector>
#include "../core/FileInfo.h"
#include "../utils/ExtensionFamilyMap.h"
#include "DuplicateEngine.h"

// Strategy 5d: Same content across extension families.
inline std::vector<DuplicateGroup> extension_family(const std::vector<FileInfo>& files) {
    // Group by SHA256 first, then check extension families.
    std::unordered_map<std::array<uint8_t, 32>, std::vector<FileInfo>> sha_groups;
    
    for (auto& f : files) {
        sha_groups[f.sha256].push_back(f);
    }

    std::vector<DuplicateGroup> groups;
    
    for (const auto& [sha, group] : sha_groups) {
        if (group.size() < 2) continue;
        
        // Check all pairs for extension family match.
        bool found_family_match = false;
        for (size_t i = 0; i < group.size(); ++i) {
            for (size_t j = i + 1; j < group.size(); ++j) {
                auto ext_a = PathUtils::get_extension(group[i].path);
                auto ext_b = PathUtils::get_extension(group[j].path);
                
                if (ext_a != ext_b && ExtensionFamilyMap::is_same_family(ext_a, ext_b)) {
                    found_family_match = true;
                    break;
                }
            }
            if (found_family_match) break;
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