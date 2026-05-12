#pragma once

#include <vector>
#include <unordered_map>
#include <string>
#include "../core/FileInfo.h"
#include "../core/Strategy.h"
#include "DuplicateEngine.h"

// Group directories with identical tree hashes (delegates to FolderCopy).
inline std::vector<DuplicateGroup> folder_copy(const std::wstring& dir_path, Sha256& out_hash) {
    return ::folder_copy({dir_path});
}