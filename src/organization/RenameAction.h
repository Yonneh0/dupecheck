#pragma once
#include <string>
#include "../core/FileInfo.h"

/// Generate a renamed path for a duplicate file (e.g., "file (copy 1).txt").
class RenameAction {
public:
    static std::wstring generate_name(const FileInfo& original, int index) {
        auto name = PathUtils::get_name_without_ext(original.path);
        return name + L" (copy " + std::to_wstring(index + 1) + L")." + PathUtils::get_extension(original.path);
    }
};