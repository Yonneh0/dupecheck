#pragma once
#include "../core/Strategy.h"
#include <string>

// Settings dialog for configuring detection thresholds and extension families.
void render_settings_dialog();
extern std::wstring g_scan_path;
StrategyConfig& get_strategy_config();