#pragma once
#include <vector>
struct DuplicateGroup;

/// Render a preview panel showing duplicate groups with action options.
void render_preview_panel(const std::vector<DuplicateGroup>& groups);
