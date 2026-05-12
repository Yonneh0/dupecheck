#pragma once

#include <vector>
#include <string>
#include "FileInfo.h"
#include "Strategy.h"

enum class FileType { Original, Duplicate };

enum class ActionType { Rename, MoveToDuplicatesFolder, Delete, CreateSymlink, Archive };

struct ActionItem {
    FileInfo file;
    ActionType action = ActionType::Rename;
    bool selected = true;
    FileType type = FileType::Original;
    std::string new_name;
    int copy_index = 0;
};

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
