#pragma once
#include <string>
#include "../core/FileInfo.h"
#include <windows.h>

// Lightweight move helper (kept for compatibility).
class MoveAction {
public:
    static bool apply(const FileInfo& file, const wchar_t* target_dir) {
        std::wstring dest = std::wstring(target_dir) + L"\\" + PathUtils::get_name_without_ext(file.path);
        return MoveFileExW(PathUtils::to_long_path(file.path).c_str(),
                           PathUtils::to_long_path(dest).c_str(), MOVEFILE_REPLACE_EXISTING) != 0;
    }

    static std::wstring get_duplicates_folder(const std::wstring& dir) {
        if (dir.empty()) return L"duplicates";
        return dir + L"\\duplicates";
    }
};