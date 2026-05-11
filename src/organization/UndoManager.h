#pragma once
#include <vector>
#include "../core/ActionModel.h"

using HistoryEntry = ActionHistoryEntry;

class UndoManager {
public:
    static void record_action(const HistoryEntry& entry);
    
    static void undo(int count = 1);
    
    static std::vector<HistoryEntry> get_history();

private:
    static std::vector<HistoryEntry> history_;
};
