#pragma once
#include <vector>
#include "../core/FileInfo.h"
#include "DuplicateEngine.h"

// Strategy 5c: Group by similar size and XxHash32 bin.
inline std::vector<DuplicateGroup> size_hash_similar(const std::vector<FileInfo>& files, uint32_t tolerance) {
    // Group files by exact size first.
    std::unordered_map<uint64_t, std::vector<FileInfo>> size_groups;
    
    for (auto& f : files) {
        size_groups[f.size].push_back(f);
    }

    std::vector<DuplicateGroup> groups;
    
    for (auto& [size, group] : size_groups) {
        if (group.size() < 2) continue;
        
        // Sort by XxHash32 value.
        auto sorted = group;
        std::sort(sorted.begin(), sorted.end(), [](const FileInfo& a, const FileInfo& b) {
            return a.xxhash < b.xxhash;
        });

        // Group into bins based on tolerance.
        size_t start_idx = 0;
        while (start_idx < sorted.size()) {
            XxHash32 bin_start = sorted[start_idx].xxhash & ~(tolerance - 1);
            
            std::vector<FileInfo> bin_group;
            for (size_t i = start_idx; i < sorted.size() && static_cast<XxHash32>(sorted[i].xxhash) - bin_start < tolerance; ++i) {
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