#pragma once
#include <string>
#include <algorithm>

class RenameAction {
public:
    static std::wstring generate_name(const FileInfo& original, const FileInfo& copy, 
                                       int index, int total_in_group) {
        auto orig_name = PathUtils::get_name_without_ext(original.path);
        auto ext = PathUtils::get_extension(original.path);

        return orig_name + L" (copy " + std::to_wstring(index + 1) + L")." + ext;
    }

    static bool apply(const FileInfo& file, const wchar_t* new_path) {
        LONG result = SHFileOperationW(&SHFILEOPSTRUCT{
            .hwnd = nullptr,
            .wFunc = FO_MOVE,
            .pFrom = file.path.c_str(),
            .pTo = new_path,
            .fFlags = FOF_NOCONFIRMATION | FOF_SILENT,
        });
        return result == 0;
    }

    static bool undo(const wchar_t* old_name, const wchar_t* new_name) {
        LONG result = SHFileOperationW(&SHFILEOPSTRUCT{
            .hwnd = nullptr,
            .wFunc = FO_MOVE,
            .pFrom = new_name,
            .pTo = old_name,
            .fFlags = FOF_NOCONFIRMATION | FOF_SILENT,
        });
        return result == 0;
    }
};
