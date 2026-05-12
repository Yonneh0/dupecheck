#pragma once
#include <string>
#include "../core/FileInfo.h"

/// Provides helper to get the duplicates subfolder path for a given file.
class MergeAction {
public:
    static std::wstring get_duplicates_folder(const FileInfo& file) {
        return PathUtils::get_parent_dir(file.path) + L"\\duplicates";
    }
};
