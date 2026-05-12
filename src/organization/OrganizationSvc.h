#pragma once
#include <vector>
#include "../core/ActionModel.h"
#include "MergeAction.h"

/// Orchestrates batch actions on duplicate file groups.
/// Supports Rename and MoveToDuplicatesFolder actions with full undo history.
class OrganizationSvc {
public:
    /// Generate a renamed path with numeric suffix (e.g., "file (1).ext").
    static std::wstring generate_renamed_path(const FileInfo& file, int index);

    /// Apply a list of actions and record history for undo.
    static void apply(std::vector<ActionItem>& items);

    /// Undo the most recent action from history.
    static void undo_actions();

    // Exposed for UI access (Undo All button).
    inline static std::vector<ActionHistoryEntry> history_;

private:
    static std::vector<ActionItem> generate_actions(const DuplicateGroup& group, ActionType action_type = ActionType::Rename);
    static void apply_actions(const std::vector<ActionItem>& items);
};