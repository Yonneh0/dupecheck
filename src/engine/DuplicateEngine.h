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

/// A group of files identified as duplicates by a single strategy.
struct DuplicateGroup {
    std::vector<FileInfo> files;   /// Files in this duplicate group.
    Strategy strategy = Strategy::ExactMatch;  /// Which detection strategy found them.
    std::string label;             /// Human-readable description (e.g., "Exact Match (3 files)").
};

/// Core engine that runs all enabled duplicate-detection strategies against scanned file data.
class DuplicateEngine {
public:
    explicit DuplicateEngine(StrategyConfig config);
    
    // Run all enabled strategies against the scanned files and merge results.
    std::vector<DuplicateGroup> find_duplicates(const std::vector<FileInfo>& files, uint32_t strategies);

    /// Register directories for folder-copy detection (used by FolderCopy strategy).
    void set_directories(const std::vector<std::wstring>& dirs) { dirs_ = dirs; }
    
private:
    StrategyConfig config_;     /// Detection configuration.
    std::vector<std::wstring> dirs_;  ///< Registered directories for folder-copy analysis.
};
