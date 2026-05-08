#pragma once
#include <windows.h>
#include "engine/DuplicateEngine.h"

// Preview panel functions — declared here, defined in PreviewPanel.cpp.
void render_preview_panel(const std::vector<DuplicateGroup>& groups);
void generate_preview_actions(const std::vector<DuplicateGroup>& groups);
void apply_all_actions();
