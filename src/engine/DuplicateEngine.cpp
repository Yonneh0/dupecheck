#include "DuplicateEngine.h"
#include <algorithm>

DuplicateEngine::DuplicateEngine(StrategyConfig config) : config_(std::move(config)) {}

std::vector<DuplicateGroup> DuplicateEngine::find_duplicates(const std::vector<FileInfo>& files, uint32_t strategies) {
    std::vector<DuplicateGroup> all_groups;
    if (strategies & static_cast<uint32_t>(Strategy::ExactMatch)) for (auto& g : exact_match(files)) all_groups.push_back(std::move(g));
    if (strategies & static_cast<uint32_t>(Strategy::NameVariant)) for (auto& g : name_variant(files, config_.name_similarity_threshold)) all_groups.push_back(std::move(g));
    if (strategies & static_cast<uint32_t>(Strategy::SizeHashSimilar)) for (auto& g : size_hash_similar(files, config_.hash_tolerance)) all_groups.push_back(std::move(g));
    if (strategies & static_cast<uint32_t>(Strategy::ExtensionFamily)) for (auto& g : extension_family(files)) all_groups.push_back(std::move(g));
    if (!dirs_.empty() && (strategies & static_cast<uint32_t>(Strategy::FolderCopy))) for (auto& g : folder_copy(dirs_)) all_groups.push_back(std::move(g));
    return all_groups;
}