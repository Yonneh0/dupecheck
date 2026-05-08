#pragma once
#include <vector>
#include "../core/ActionModel.h"  // Reuses ActionHistoryEntry from ActionModel.h

// Simple history stack for undo support.
// Aliases the ActionHistoryEntry defined in ActionModel.h to avoid duplication.
using HistoryEntry = ActionHistoryEntry;

class UndoManager {
public:
    static void record_action(const HistoryEntry& entry);
    
    // Undo the last N actions.
    static void undo(int count = 1);
    
    // Get all recorded history entries.
    static std::vector<HistoryEntry> get_history();

private:
    static std::vector<HistoryEntry> history_;
};