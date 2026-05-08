#pragma once
#include <string>
#include "../core/FileInfo.h"

class DeleteAction {
public:
    // Delete a file from disk.
    static bool apply(const FileInfo& file) {
        BOOL result = DeleteFileW(PathUtils::to_long_path(file.path).c_str());
        return result != 0;
    }

    // Restore a previously deleted file from its backup data.
    static bool undo(const std::wstring& original_path, const std::vector<uint8_t>& data) {
        HANDLE h = CreateFileW(PathUtils::to_long_path(original_path).c_str(), 
                               GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        
        if (h == INVALID_HANDLE_VALUE) return false;
        
        DWORD bytes_written;
        WriteFile(h, data.data(), static_cast<DWORD>(data.size()), &bytes_written, nullptr);
        CloseHandle(h);
        
        return true;
    }
};
