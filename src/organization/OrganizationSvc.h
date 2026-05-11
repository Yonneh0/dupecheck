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
    static std::vector<ActionItem> generate_actions(const DuplicateGroup& group, ActionType action_type);
    static std::vector<ActionItem> generate_actions(const std::vector<DuplicateGroup>& groups,
                                                    ActionType action_type = ActionType::Rename);
    static std::wstring generate_renamed_path(const FileInfo& file, int index);
    static void apply(std::vector<ActionItem>& items);
    static void undo_actions();

private:
    inline static std::vector<ActionHistoryEntry> history_;
};