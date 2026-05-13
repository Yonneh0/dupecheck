#pragma once
#include <vector>
#include "../core/ActionModel.h"
#include "../engine/DuplicateEngine.h"
#include "MergeAction.h"

/// Orchestrates batch actions on duplicate file groups.
/// Supports Rename and MoveToDuplicatesFolder actions with full undo history.
class OrganizationSvc {
public:
    /// Generate a renamed path with numeric suffix (e.g., "file (1).ext").
    static std::wstring generate_renamed_path(const FileInfo& file, int index);

    /// Apply a list of actions and record history for undo.
    static void apply(std::vector<ActionItem>& items);

    /// Undo one action, or all if count <= 0.
    static void undo_actions(int count = 1);

    /// Clear the entire action history.
    static void clear_history();

    /// Get a reference to the action history (for UI access).
    inline static std::vector<ActionHistoryEntry>& get_history() { return history_; }

private:
    static void redo_one_action(const ActionHistoryEntry& entry);
    static void apply_actions(const std::vector<ActionItem>& items);

    /// Generate action items for a single duplicate group.
    static std::vector<ActionItem> generate_actions(const DuplicateGroup& group, ActionType action_type = ActionType::Rename);

    /// Generate action items for multiple duplicate groups.
    static std::vector<ActionItem> generate_actions(const std::vector<DuplicateGroup>& groups, ActionType action_type = ActionType::Rename);
};
