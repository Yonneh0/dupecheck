#pragma once
#include <vector>
#include "../core/FileInfo.h"
#include "../utils/Levenshtein.h"
#include "DuplicateEngine.h"

inline std::vector<DuplicateGroup> name_variant(const std::vector<FileInfo>& files, int threshold) {
    std::unordered_map<std::array<uint8_t, 32>, std::vector<FileInfo>> sha_groups;
    
    for (auto& f : files) {
        sha_groups[f.sha256].push_back(std::move(f));
    }

    std::vector<DuplicateGroup> groups;
    
    for (auto& [sha, group] : sha_groups) {
        if (group.size() < 2) continue;
        
        std::unordered_map<int, int> name_clusters;
        bool found_variant = false;
        
        for (size_t i = 0; i < group.size(); ++i) {
            const auto& a_name = PathUtils::get_name_without_ext(group[i].path);
            
            for (size_t j = i + 1; j < group.size(); ++j) {
                const auto& b_name = PathUtils::get_name_without_ext(group[j].path);
                
                int dist = levenshtein_distance(a_name, b_name);
                if (dist <= threshold) {
                    found_variant = true;
                }
            }
        }

        if (found_variant && group.size() > 1) {
            DuplicateGroup dg;
            dg.files = std::move(group);
            dg.strategy = Strategy::NameVariant;
            dg.label = "Name Variants (" + std::to_string(dg.files.size()) + " files)";
            groups.push_back(std::move(dg));
        }
    }

    return groups;
}
