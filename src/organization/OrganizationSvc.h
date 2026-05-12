#pragma once
#include <vector>
#include "../core/ActionModel.h"
#include "RenameAction.h"
#include "MoveAction.h"
#include "DeleteAction.h"

// Orchestrates batch actions on duplicate file groups (rename, move, delete, etc.).
class OrganizationSvc {
public:
    static std::wstring generate_renamed_path(const FileInfo& file, int index);
    static void apply(std::vector<ActionItem>& items);
    static void undo_actions();

private:
    // Generate action items for a single duplicate group.
    static std::vector<ActionItem> generate_actions(const DuplicateGroup& group, ActionType action_type = ActionType::Rename);
    static void apply_actions(const std::vector<ActionItem>& items);

    inline static std::vector<ActionHistoryEntry> history_;
};