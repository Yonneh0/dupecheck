#pragma once
#include <vector>
#include "../engine/DuplicateEngine.h"

/// Render a preview panel showing duplicate groups with action options.
void render_preview_panel(const std::vector<DuplicateGroup>& groups);