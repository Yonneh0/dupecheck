#pragma once
#include <vector>
#include "../core/FileInfo.h"
#include "../core/Strategy.h"
#include "../core/ActionModel.h"
#include "ExactMatch.h"
#include "NameVariant.h"
#include "SizeHashSimilar.h"
#include "ExtensionFamily.h"
#include "FolderCopy.h"

struct DuplicateGroup {
    std::vector<FileInfo> files;
    Strategy strategy = Strategy::ExactMatch;
    std::string label;
};

class DuplicateEngine {
public:
    explicit DuplicateEngine(StrategyConfig config);
    std::vector<DuplicateGroup> find_duplicates(const std::vector<FileInfo>& files, uint32_t strategies);
    void set_directories(const std::vector<std::wstring>& dirs) { dirs_ = dirs; }

private:
    StrategyConfig config_;
    std::vector<std::wstring> dirs_;
};