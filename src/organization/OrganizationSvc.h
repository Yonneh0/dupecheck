#pragma once
#include <vector>
#include "../core/ActionModel.h"
#include "MergeAction.h"

/// Orchestrates batch actions on duplicate file groups (rename, move, delete).
class OrganizationSvc {
public:
    static std::wstring generate_renamed_path(const FileInfo& file, int index);
    /// Apply a list of actions.
    static void apply(std::vector<ActionItem>& items);
    /// Undo the last action.
    static void undo_actions();

    // Exposed for UI access (Undo All button).
    inline static std::vector<ActionHistoryEntry> history_;

private:
    static std::vector<ActionItem> generate_actions(const DuplicateGroup& group, ActionType action_type = ActionType::Rename);
    static void apply_actions(const std::vector<ActionItem>& items);
};