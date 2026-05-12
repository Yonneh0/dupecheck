#pragma once
#include <string>
#include "../core/FileInfo.h"
#include <windows.h>

/// Move a file into the duplicates subfolder of its parent directory.
class MergeAction {
public:
    static std::wstring get_duplicates_folder(const FileInfo& file) {
        return PathUtils::get_parent_dir(file.path) + L"\\duplicates";
    }

    static bool apply(const FileInfo& file, const std::wstring& target_dir) {
        auto full_name = PathUtils::get_name_without_ext(file.path) + L"." + PathUtils::get_extension(file.path);
        return MoveFileExW(PathUtils::to_long_path(file.path).c_str(),
                           PathUtils::to_long_path(target_dir + L"\\" + full_name).c_str(),
                           MOVEFILE_REPLACE_EXISTING) != 0;
    }

    static bool undo(const std::wstring& old_path, const std::wstring& new_path) {
        return MoveFileExW(PathUtils::to_long_path(new_path).c_str(),
                           PathUtils::to_long_path(old_path).c_str(), MOVEFILE_REPLACE_EXISTING) != 0;
    }
};
