#pragma once
#include "../core/Strategy.h"
#include <string>
#include <vector>

// Strategy label array (used by SettingsDialog).
constexpr const char* STRATEGY_LABELS[] = {
    "Exact Match",
    "Name Variant",
    "Size+Hash Similar",
    "Extension Family",
    "Folder Copy"
};

// Render the settings dialog (modal).
void render_settings_dialog();

// Global config variables used by GUI and SettingsDialog.
extern std::wstring g_scan_path;

// Access module-level config defined in ImGuiView.cpp.
StrategyConfig& get_strategy_config();