#pragma once
#include <string>
#include "../core/FileInfo.h"
#include <windows.h>

// Lightweight archive helper (kept for compatibility).
class ArchiveAction {
public:
    static bool apply(const std::vector<FileInfo>& group, const wchar_t* output_path) {
        SHFILEOPSTRUCTW fo{};
        fo.wFunc = FO_COPY;
        fo.pFrom = group[0].path.c_str();
        fo.pTo = output_path;
        fo.fFlags = FOF_NOCONFIRMATION | FOF_SILENT;
        return SHFileOperationW(&fo) == 0;
    }

    static bool create_zip(const std::vector<std::wstring>& files, const wchar_t* path) {
        HANDLE h = CreateFileW(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING, nullptr);
        if (h == INVALID_HANDLE_VALUE) return false;
        DWORD bytes_written;
        WriteFile(h, "PK\x05\x06", 22, &bytes_written, nullptr);
        CloseHandle(h);
        return true;
    }

    static bool undo(const std::wstring& archive_path) {
        return DeleteFileW(PathUtils::to_long_path(archive_path).c_str()) != 0;
    }
};