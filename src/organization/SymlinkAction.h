#pragma once
#include <string>
#include "../core/FileInfo.h"

class SymlinkAction {
public:
    static bool apply(const FileInfo& file) {
        std::wstring original = PathUtils::to_long_path(file.path);
        
        // Create symbolic link using Windows API.
        BOOL result = CreateSymbolicLinkW(
            PathUtils::to_long_path(L"temp_link").c_str(),
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