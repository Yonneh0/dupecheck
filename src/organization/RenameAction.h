#pragma once
#include <string>
#include "../core/FileInfo.h"

// Lightweight rename helper (kept for compatibility).
class RenameAction {
public:
    static std::wstring generate_name(const FileInfo& original, int index) {
        auto name = PathUtils::get_name_without_ext(original.path);
        return name + L" (copy " + std::to_wstring(index + 1) + L")." + PathUtils::get_extension(original.path);
    }
};