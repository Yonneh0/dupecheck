#pragma once
#include "../core/Strategy.h"

/// Get or modify the global strategy configuration (shared between GUI and service).
inline StrategyConfig& get_strategy_config_impl() {
    static StrategyConfig config{3, 1024};
    return config;
}

void render_settings_dialog();