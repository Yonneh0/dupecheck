#pragma once
#include <vector>
#include "../core/FileInfo.h"
#include "../core/Strategy.h"
#include "DuplicateGroup.h"
#include "ExactMatch.h"
#include "NameVariant.h"
#include "SizeHashSimilar.h"
#include "ExtensionFamily.h"
#include "FolderCopy.h"

/// Merge results from multiple strategies, removing files that appear in lower-priority groups.
std::vector<DuplicateGroup> deduplicate_groups(std::vector<DuplicateGroup>&& groups);

/// Run all enabled detection strategies against the given file list and return deduplicated results.
class DuplicateEngine {
public:
    explicit DuplicateEngine(StrategyConfig config = {});
    std::vector<DuplicateGroup> find_duplicates(const std::vector<FileInfo>& files, uint32_t strategies);
    void set_directories(const std::vector<std::wstring>& dirs) noexcept { dirs_ = dirs; }

private:
    StrategyConfig config_;
    std::vector<std::wstring> dirs_;
};
