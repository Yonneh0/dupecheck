#pragma once
#include <vector>
#include "../core/FileInfo.h"
#include "../core/Strategy.h"

struct DuplicateGroup {
    std::vector<FileInfo> files;
    Strategy strategy = Strategy::ExactMatch;
    std::string label;
};