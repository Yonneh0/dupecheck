#pragma once
#include "../core/Strategy.h"

/// Get or modify the global strategy configuration (shared between GUI and service).
StrategyConfig& get_strategy_config_impl();
void render_settings_dialog();