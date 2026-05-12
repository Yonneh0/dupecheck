#pragma once
#include <string>
#include "../core/FileInfo.h"
#include <windows.h>

// Delete a file from disk using long-path support.
class DeleteAction {
public:
    static bool apply(const FileInfo& file) {
        return DeleteFileW(PathUtils::to_long_path(file.path).c_str()) != 0;
    }

    // Undo — restores the deleted file (no-op placeholder since true restore requires backup metadata).
    static bool undo(const std::wstring&) { return true; }
};