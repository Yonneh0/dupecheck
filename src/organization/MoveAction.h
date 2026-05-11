#pragma once
#include <string>
#include "../core/FileInfo.h"

class MoveAction {
public:
    static bool apply(const FileInfo& file, const wchar_t* target_dir) {
        std::wstring dest = std::wstring(target_dir) + L"\\" + PathUtils::get_name_without_ext(file.path);
        SHFILEOPSTRUCT fo{}; fo.hwnd = nullptr; fo.wFunc = FO_MOVE;
        fo.pFrom = file.path.c_str(); fo.pTo = dest.c_str();
        fo.fFlags = FOF_NOCONFIRMATION | FOF_SILENT; return SHFileOperationW(&fo) == 0;
    }
    static std::wstring get_duplicates_folder(const std::wstring& dir) { return PathUtils::get_duplicates_subfolder(dir); }
};