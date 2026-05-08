#include "DuplicateEngine.h"
#include <algorithm>

DuplicateEngine::DuplicateEngine(StrategyConfig config) : config_(std::move(config)) {}

std::vector<DuplicateGroup> DuplicateEngine::find_duplicates(const std::vector<FileInfo>& files, uint32_t strategies) {
    std::vector<DuplicateGroup> all_groups;

    // Strategy: Exact Match.
    if (strategies & static_cast<uint32_t>(Strategy::ExactMatch)) {
        auto groups = exact_match(files);
        for (auto& g : groups) {
            all_groups.push_back(std::move(g));
        }
    }

    // Strategy: Name Variant.
    if (strategies & static_cast<uint32_t>(Strategy::NameVariant)) {
        auto groups = name_variant(files, config_.name_similarity_threshold);
        for (auto& g : groups) all_groups.push_back(std::move(g));
    }

    // Strategy: Size+Hash Similar.
    if (strategies & static_cast<uint32_t>(Strategy::SizeHashSimilar)) {
        auto groups = size_hash_similar(files, config_.hash_tolerance);
        for (auto& g : groups) all_groups.push_back(std::move(g));
    }

    // Strategy: Extension Family.
    if (strategies & static_cast<uint32_t>(Strategy::ExtensionFamily)) {
        auto groups = extension_family(files);
        for (auto& g : groups) all_groups.push_back(std::move(g));
    }

    // Strategy: Folder Copy — only run if we have directory registrations.
    if (!dirs_.empty() && (strategies & static_cast<uint32_t>(Strategy::FolderCopy))) {
        auto groups = folder_copy(dirs_);
        for (auto& g : groups) all_groups.push_back(std::move(g));
    }

    return all_groups;
}
