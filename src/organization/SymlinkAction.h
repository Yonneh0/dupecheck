#pragma once
#include <string>
#include "../core/FileInfo.h"

class SymlinkAction {
public:
    // Create a symbolic link for the given file.
    // The symlink is named "_<filename>.link" in the same directory as the original.
    static bool apply(const FileInfo& file) {
        std::wstring parent_dir = PathUtils::get_parent_dir(file.path);
        std::wstring base_name = PathUtils::get_name_without_ext(file.path);
        std::wstring symlink_path = parent_dir + L"\\" + base_name + L".link";

        std::wstring original = PathUtils::to_long_path(file.path);
        BOOL result = CreateSymbolicLinkW(
            PathUtils::to_long_path(symlink_path).c_str(),
            original.c_str(),
            FILE_ATTRIBUTE_NORMAL
        );

        return result != 0;
    }

    static bool undo(const std::wstring& symlink_path) {
        BOOL result = DeleteFileW(PathUtils::to_long_path(symlink_path).c_str());
        return result != 0;
    }
};
