#pragma once
#include <string>
#include "../core/FileInfo.h"
#include <windows.h>

// Lightweight symlink helper (kept for compatibility).
class SymlinkAction {
public:
    static bool apply(const FileInfo& file) {
        std::wstring link_name = PathUtils::get_parent_dir(file.path) + L"\\" +
                                 PathUtils::get_name_without_ext(file.path) + L".link";
        return CreateSymbolicLinkW(PathUtils::to_long_path(link_name).c_str(),
                                   PathUtils::to_long_path(file.path).c_str(),
                                   FILE_ATTRIBUTE_NORMAL) != 0;
    }

    static bool undo(const std::wstring& symlink_path) {
        return DeleteFileW(PathUtils::to_long_path(symlink_path).c_str()) != 0;
    }
};