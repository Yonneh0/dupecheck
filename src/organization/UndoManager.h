#pragma once
#include <vector>
#include "../core/ActionModel.h"

class UndoManager {
public:
    static void record_action(const ActionHistoryEntry& entry);
    static void undo(int count = 1);
    static std::vector<ActionHistoryEntry> get_history();
    static void clear();

private:
    inline static std::vector<ActionHistoryEntry> history_;
};