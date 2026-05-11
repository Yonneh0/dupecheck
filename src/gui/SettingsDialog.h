#pragma once
#include "../core/Strategy.h"
#include <string>
#include <vector>

constexpr const char* STRATEGY_LABELS[] = {
    "Exact Match",
    "Name Variant",
    "Size+Hash Similar",
    "Extension Family",
    "Folder Copy"
};

void render_settings_dialog();

extern std::wstring g_scan_path;

StrategyConfig& get_strategy_config();
