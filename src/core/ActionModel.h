#pragma once

#include <vector>
#include <string>
#include "FileInfo.h"
#include "Strategy.h"
#include "../engine/DuplicateEngine.h"
#include "../engine/FolderCopy.h"

// File type for action items.
enum class FileType { Original, Duplicate };

// Actions that can be applied to duplicate files.
enum class ActionType { Rename, MoveToDuplicatesFolder, Delete, CreateSymlink, Archive };

struct ActionItem {
    FileInfo file;
    ActionType action = ActionType::Rename;
    bool selected = true;
    FileType type = FileType::Original;
    std::string new_name;
    int copy_index = 0;
};

// History entry for undo support.
struct ActionHistoryEntry {
    std::wstring file_path;
    ActionType action_type = ActionType::Rename;
    std::string old_value;
    std::string new_value;
    std::wstring backup_path;
};

enum class CliCommand { None, InstallService, UninstallService, RunService };

struct ServiceArgs {
    std::string scan_path;
    bool installed = false;
    CliCommand command = CliCommand::None;
};

// Group directories with identical tree hashes (delegates to FolderCopy).
inline std::vector<DuplicateGroup> folder_copy(const wchar_t* dir_path, Sha256& out_hash) {
    return ::folder_copy({dir_path}, out_hash);
}