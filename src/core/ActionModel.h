#pragma once
#include <string>
#include <vector>
#include "FileInfo.h"
#include <cstdint>

/// Represents whether a file is an original or duplicate in a group.
enum class FileType {
    Original,
    Duplicate
};

/// Action to perform on a duplicate file.
enum class ActionType {
    Rename,                  /// Rename the file (e.g., add "(copy 1)" suffix).
    MoveToDuplicatesFolder,  /// Move to <parent>/duplicates/ subfolder.
    Delete,                  /// Delete from disk.
    CreateSymlink,           /// Replace with a symbolic link pointing back to original.
    Archive,                 /// Zip/archive the file.
};

/// Convert ActionType enum to string for undo history.

struct ActionItem {
    FileInfo file;
    FileType type = FileType::Duplicate;
    std::string new_name;
    ActionType action = ActionType::Rename;
    bool selected = false;
};

inline const char* action_type_to_string(ActionType t) {
    switch (t) {
        case ActionType::Rename:                  return "rename";
        case ActionType::MoveToDuplicatesFolder:  return "move";
        case ActionType::Delete:                  return "delete";
        case ActionType::CreateSymlink:           return "symlink";
        case ActionType::Archive:                 return "archive";
    }
    return "unknown";
}

/// Convert FileType enum to human-readable string.
inline const char* file_type_to_string(FileType t) {
    switch (t) {
        case FileType::Original:  return "original";
        case FileType::Duplicate: return "duplicate";
    }
    return "unknown";
}


/// History entry for tracking actions so they can be undone later.
struct ActionHistoryEntry {
    std::wstring file_path;         /// Path of the file affected by the action.
    ActionType action_type = ActionType::Rename;  /// Type of action performed.
    std::string old_value;          /// Previous value (e.g., old path or name).
    std::string new_value;          /// New value after action.
    std::wstring backup_path;       /// For delete undo: original path to restore from.
    bool performed = true;          /// Whether this action has been applied.
};

