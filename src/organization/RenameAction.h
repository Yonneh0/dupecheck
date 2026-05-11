#pragma once
#include <string>
#include "../core/FileInfo.h"

class RenameAction {
public:
    static std::wstring generate_name(const FileInfo& original, const FileInfo& copy, int index, int total_in_group) {
        auto orig_name = PathUtils::get_name_without_ext(original.path);
        return orig_name + L" (copy " + std::to_wstring(index + 1) + L")." + PathUtils::get_extension(original.path);
    }

    static bool apply(const FileInfo& file, const wchar_t* new_path) {
        SHFILEOPSTRUCT fo{}; fo.hwnd = nullptr; fo.wFunc = FO_MOVE; fo.pFrom = file.path.c_str();
        fo.pTo = new_path; fo.fFlags = FOF_NOCONFIRMATION | FOF_SILENT;
        return SHFileOperationW(&fo) == 0;
    }

    static bool undo(const wchar_t* old_name, const wchar_t* new_name) {
        SHFILEOPSTRUCT fo{}; fo.hwnd = nullptr; fo.wFunc = FO_MOVE; fo.pFrom = new_name;
        fo.pTo = old_name; fo.fFlags = FOF_NOCONFIRMATION | FOF_SILENT;
        return SHFileOperationW(&fo) == 0;
    }
};