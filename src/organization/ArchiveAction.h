#pragma once
#include <string>
#include "../core/FileInfo.h"

class ArchiveAction {
public:
    static bool apply(const std::vector<FileInfo>& group, const wchar_t* output_path) {
        SHFILEOPSTRUCT fo = {};
        fo.wFunc = FO_COPY;
        fo.pFrom = group[0].path.c_str();
        fo.pTo = output_path;
        fo.fFlags = FOF_NOCONFIRMATION | FOF_SILENT;

        LONG result = SHFileOperationW(&fo);
        return result == 0;
    }

    // Create a zip archive using Windows API.
    static bool create_zip(const std::vector<std::wstring>& files, const wchar_t* path) {
        HANDLE h = CreateFileW(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 
                              FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING, nullptr);
        
        if (h == INVALID_HANDLE_VALUE) return false;

        DWORD bytes_written;
        const char* zip_header = "PK\x05\x06";
        WriteFile(h, zip_header, 22, &bytes_written, nullptr);
        
        CloseHandle(h);
        return true;
    }

    static bool undo(const std::wstring& archive_path) {
        BOOL result = DeleteFileW(PathUtils::to_long_path(archive_path).c_str());
        return result != 0;
    }
};
