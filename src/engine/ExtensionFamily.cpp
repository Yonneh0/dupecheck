#include "ExtensionFamily.h"
#include <unordered_map>
#include "../utils/ExtensionFamilyMap.h"

std::vector<DuplicateGroup> extension_family(const std::vector<FileInfo>& files) {
    // Only keep groups that have at least one cross-family extension pair.
    std::unordered_map<std::array<uint8_t, 32>, std::vector<FileInfo>> sha_groups;
    for (const auto& f : files) sha_groups[f.sha256].push_back(f);

    std::vector<DuplicateGroup> groups;
    for (auto& [sha, group] : sha_groups) {
        if (group.size() < 2) continue;

        bool found_family_match = false;
        for (size_t i = 0; i < group.size() && !found_family_match; ++i) {
            std::wstring ext_i = PathUtils::get_extension(group[i].path);
            for (size_t j = i + 1; j < group.size(); ++j) {
                std::wstring ext_j = PathUtils::get_extension(group[j].path);
                if (ext_i != ext_j && ExtensionFamilyMap::is_same_family(ext_i, ext_j)) {
                    found_family_match = true;
                    break;
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