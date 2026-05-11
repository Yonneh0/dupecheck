#pragma once
#include <vector>
#include "../core/FileInfo.h"
#include "DuplicateEngine.h"

inline std::vector<DuplicateGroup> size_hash_similar(const std::vector<FileInfo>& files, uint32_t tolerance) {
    std::unordered_map<uint64_t, std::vector<FileInfo>> size_groups;
    
    for (auto& f : files) {
        size_groups[f.size].push_back(f);
    }

    std::vector<DuplicateGroup> groups;
    
    for (auto& [size, group] : size_groups) {
        if (group.size() < 2) continue;
        
        auto sorted = group;
        std::sort(sorted.begin(), sorted.end(), [](const FileInfo& a, const FileInfo& b) {
            return a.xxhash < b.xxhash;
        });

        size_t start_idx = 0;
        while (start_idx < sorted.size()) {
            XxHash32 bin_start = sorted[start_idx].xxhash & ~(tolerance - 1);
            
            std::vector<FileInfo> bin_group;
            for (size_t i = start_idx; i < sorted.size() && static_cast<int64_t>(sorted[i].xxhash) - static_cast<int64_t>(bin_start) < static_cast<int64_t>(tolerance); ++i) {
                bin_group.push_back(sorted[i]);
            }

            if (bin_group.size() > 1) {
                DuplicateGroup dg;
                for (auto& f : bin_group) dg.files.push_back(std::move(f));
                dg.strategy = Strategy::SizeHashSimilar;
                dg.label = "Size+Hash Similar (" + std::to_string(dg.files.size()) + " files, tolerance=" + std::to_string(tolerance) + ")";
                groups.push_back(std::move(dg));
            }

            start_idx += bin_group.empty() ? 1 : bin_group.size();
        }
    }

    return groups;
}
