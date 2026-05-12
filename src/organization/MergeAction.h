#pragma once
#include <string>
#include "../core/FileInfo.h"
#include <windows.h>

// =============================================================================
// File operation helpers for batch actions on duplicate groups.
// Each class provides apply() and undo() static methods.
// =============================================================================

/// Move a file into the duplicates subfolder of its parent directory.
class MergeAction {
public:
    static std::wstring get_duplicates_folder(const FileInfo& file) {
        return PathUtils::get_parent_dir(file.path) + L"\\duplicates";
    }

    static bool apply(const FileInfo& file, const std::wstring& target_dir) {
        auto full_name = PathUtils::get_name_without_ext(file.path) + L"." + PathUtils::get_extension(file.path);
        return MoveFileExW(PathUtils::to_long_path(file.path).c_str(),
                           PathUtils::to_long_path(target_dir + L"\\" + full_name).c_str(),
                           MOVEFILE_REPLACE_EXISTING) != 0;
    }

    static bool undo(const std::wstring& old_path, const std::wstring& new_path) {
        return MoveFileExW(PathUtils::to_long_path(new_path).c_str(),
                           PathUtils::to_long_path(old_path).c_str(), MOVEFILE_REPLACE_EXISTING) != 0;
    }
};

/// Archive a file into the duplicates subfolder (copy operation).
class ArchiveAction {
public:
    static bool apply(const FileInfo& file, const wchar_t* output_path) {
        SHFILEOPSTRUCTW fo{};
        fo.wFunc = FO_COPY;
        fo.pFrom = file.path.c_str();
        fo.pTo = output_path;
        fo.fFlags = FOF_NOCONFIRMATION | FOF_SILENT;
        return SHFileOperationW(&fo) == 0;
    }

    /// Create a minimal ZIP (PK header + empty central directory).
    static bool create_zip(const std::vector<std::wstring>& files, const wchar_t* path) {
        HANDLE h = CreateFileW(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING, nullptr);
        if (h == INVALID_HANDLE_VALUE) return false;

        const char eocd[] = { 'P','K', 0x05, 0x06, 0,0,0,0, 0,0,0,0, 0,0,0,0 };
        DWORD bw = 0;
        WriteFile(h, eocd, sizeof(eocd), &bw, nullptr);
        CloseHandle(h);
        return true;
    }

    static bool undo(const std::wstring& archive_path) {
        return DeleteFileW(PathUtils::to_long_path(archive_path).c_str()) != 0;
    }
};

/// Delete a file from disk.
class DeleteAction {
public:
    static bool apply(const FileInfo& file) {
        return DeleteFileW(PathUtils::to_long_path(file.path).c_str()) != 0;
    }
    static bool undo(const std::wstring&) { return true; }
};

/// Create a symbolic link for the given file.
class SymlinkAction {
public:
    static bool apply(const FileInfo& file) {
        std::wstring link_name = PathUtils::get_parent_dir(file.path) + L"\\" +
                                 PathUtils::get_name_without_ext(file.path) + L".link";
        return CreateSymbolicLinkW(PathUtils::to_long_path(link_name).c_str(),
                                   PathUtils::to_long_path(file.path).c_str(), FILE_ATTRIBUTE_NORMAL) != 0;
    }

    static bool undo(const std::wstring& symlink_path) {
        return DeleteFileW(PathUtils::to_long_path(symlink_path).c_str()) != 0;
    }
};

/// Move a file to the target directory.
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
