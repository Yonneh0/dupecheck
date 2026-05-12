#pragma once
#include <vector>
#include <string>
#include "FileInfo.h"
#include "Strategy.h"

enum class FileType { Original, Duplicate };

/// Actions supported by the organization service.
enum class ActionType { Rename, MoveToDuplicatesFolder, Delete, CreateSymlink, Archive };

struct ActionItem {
    FileInfo file;
    ActionType action = ActionType::Rename;
    bool selected = true;
    FileType type = FileType::Original;
    std::string new_name;
    int copy_index = 0;
};

/// History entry for undo support. All string fields use wstring for path consistency;
/// UTF-8 is used only when storing non-path values (action types, display names).
struct ActionHistoryEntry {
    std::wstring file_path;       // Original file path (wide char for Windows API compatibility)
    ActionType action_type = ActionType::Rename;
    std::string old_value;        // UTF-8 encoded old value (for non-path data)
    std::string new_value;        // UTF-8 encoded new value (for non-path data)
    std::wstring backup_path;     // Backup/destination path for undo operations
};

enum class CliCommand { None, InstallService, UninstallService, RunService };

struct ServiceArgs {
    std::string scan_path;
    bool installed = false;
    CliCommand command = CliCommand::None;
};