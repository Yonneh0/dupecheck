#include "DuplicateEngine.h"
#include <algorithm>

// Priority order: ExactMatch > NameVariant > SizeHashSimilar > ExtensionFamily > FolderCopy.
static uint32_t strategy_priority(Strategy s) {
    switch (s) {
        case Strategy::ExactMatch:      return 0;
        case Strategy::NameVariant:     return 1;
        case Strategy::SizeHashSimilar: return 2;
        case Strategy::ExtensionFamily: return 3;
        case Strategy::FolderCopy:      return 4;
    }
    return 5;
}

std::vector<DuplicateGroup> deduplicate_groups(std::vector<DuplicateGroup>&& groups) {
    // Sort by strategy priority so higher-priority groups are processed first.
    std::sort(groups.begin(), groups.end(), [](const DuplicateGroup& a, const DuplicateGroup& b) {
        return strategy_priority(a.strategy) < strategy_priority(b.strategy);
    });

    std::unordered_set<std::wstring> seen_paths;
    for (auto& g : groups) {
        auto it = std::remove_if(g.files.begin(), g.files.end(), [&seen_paths](const FileInfo& f) {
            return !seen_paths.insert(f.path).second;
        });
        g.files.erase(it, g.files.end());
    }

    // Remove empty groups.
    groups.erase(
        std::remove_if(groups.begin(), groups.end(), [](const DuplicateGroup& g) { return g.files.empty(); }),
        groups.end());
    return groups;
}

DuplicateEngine::DuplicateEngine(StrategyConfig config) : config_(std::move(config)) {}

std::vector<DuplicateGroup> DuplicateEngine::find_duplicates(const std::vector<FileInfo>& files, uint32_t strategies) {
    std::vector<DuplicateGroup> all_groups;
    if (strategies & static_cast<uint32_t>(Strategy::ExactMatch)) for (auto& g : exact_match(files)) all_groups.push_back(std::move(g));
    if (strategies & static_cast<uint32_t>(Strategy::NameVariant)) for (auto& g : name_variant(files, config_.name_similarity_threshold)) all_groups.push_back(std::move(g));
    if (strategies & static_cast<uint32_t>(Strategy::SizeHashSimilar)) for (auto& g : size_hash_similar(files, config_.hash_tolerance)) all_groups.push_back(std::move(g));
    if (strategies & static_cast<uint32_t>(Strategy::ExtensionFamily)) for (auto& g : extension_family(files)) all_groups.push_back(std::move(g));
    if (!dirs_.empty() && (strategies & static_cast<uint32_t>(Strategy::FolderCopy))) for (auto& g : folder_copy(dirs_)) all_groups.push_back(std::move(g));
    return deduplicate_groups(std::move(all_groups));
}
