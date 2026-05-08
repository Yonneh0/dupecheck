#pragma once
#include <vector>
#include "../core/ActionModel.h"
#include "RenameAction.h"
#include "MoveAction.h"
#include "DeleteAction.h"
#include "SymlinkAction.h"
#include "ArchiveAction.h"

class OrganizationSvc {
public:
    // Generate action items for a duplicate group.
    static std::vector<ActionItem> generate_actions(const DuplicateGroup& group, ActionType action_type);

    // Generate action items for multiple groups (for PreviewPanel).
    static std::vector<ActionItem> generate_actions(const std::vector<DuplicateGroup>& groups,
                                                    ActionType action_type = ActionType::Rename);

    // Apply all selected actions.
    static void apply(std::vector<ActionItem>& items);
    static void apply_actions(const std::vector<ActionItem>& items);

    // Undo the last batch of actions.
    static void undo_actions();

private:
    using HistoryEntry = ActionHistoryEntry;
    static std::vector<HistoryEntry> history_;
};