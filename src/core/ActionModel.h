#pragma once
#include <string>
#include <vector>
#include "FileInfo.h"
#include <cstdint>

enum class FileType {
    Original,
    Duplicate
};

enum class ActionType {
    Rename,
    MoveToDuplicatesFolder,
    Delete,
    CreateSymlink,
    Archive,
};

struct ActionItem {
    FileInfo file;
    FileType type = FileType::Duplicate;
    std::string new_name;
    int copy_index = 0;
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

inline const char* file_type_to_string(FileType t) {
    switch (t) {
        case FileType::Original:  return "original";
        case FileType::Duplicate: return "duplicate";
    }
    return "unknown";
}


struct ActionHistoryEntry {
    std::wstring file_path;
    ActionType action_type = ActionType::Rename;
    std::string old_value;
    std::string new_value;
    std::wstring backup_path;
    bool performed = true;
};

