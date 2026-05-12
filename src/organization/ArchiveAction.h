#pragma once
#include <string>
#include "../core/FileInfo.h"
#include <windows.h>

// Lightweight archive helper — copies a file into the given output path.
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

    // Create a minimal ZIP file (PK header + empty central directory).
    static bool create_zip(const std::vector<std::wstring>& files, const wchar_t* path) {
        HANDLE h = CreateFileW(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING, nullptr);
        if (h == INVALID_HANDLE_VALUE) return false;

        // PK\x05\x06 — End of Central Directory Signature.
        const char eocd[] = { 'P','K', 0x05, 0x06, 0,0,0,0, 0,0,0,0, 0,0,0,0,
                              static_cast<char>(files.size() & 0xFF),
                              static_cast<char>((files.size() >> 8) & 0xFF),
                              0,0,0,0 };
        DWORD bytes_written = 0;
        WriteFile(h, eocd, sizeof(eocd), &bytes_written, nullptr);
        CloseHandle(h);
        return true;
    }

    static bool undo(const std::wstring& archive_path) {
        return DeleteFileW(PathUtils::to_long_path(archive_path).c_str()) != 0;
    }
};