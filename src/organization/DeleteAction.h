#pragma once
#include <string>
#include "../core/FileInfo.h"

// Lightweight delete helper (kept for compatibility).
class DeleteAction {
public:
    static bool apply(const FileInfo& file) {
        return DeleteFileW(PathUtils::to_long_path(file.path).c_str()) != 0;
    }
};