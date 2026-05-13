#pragma once
#include "../core/Strategy.h"

/// Get or modify the global strategy configuration used by both the GUI and service.
inline StrategyConfig& get_strategy_config() noexcept {
    static StrategyConfig s_config{};
    return s_config;
}

void render_settings_dialog();
