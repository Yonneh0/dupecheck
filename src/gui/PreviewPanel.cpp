#include <imgui.h>
#include <imgui_impl_win32.h>
#include "../core/ActionModel.h"
#include "../organization/OrganizationSvc.h"
#include "ImGuiView.h"

// Render preview panel showing proposed actions.
void render_preview_panel(const std::vector<DuplicateGroup>& groups) {
    if (ImGui::TreeNode("Preview")) {
        for (const auto& group : groups) {
            ImGui::Indent();

            // Show group header.
            const char* strategy_name = nullptr;
            switch (group.strategy) {
                case Strategy::ExactMatch:      strategy_name = "Exact Match"; break;
                case Strategy::NameVariant:     strategy_name = "Name Variant";  break;
                case Strategy::SizeHashSimilar: strategy_name = "Size+Hash";     break;
                case Strategy::ExtensionFamily: strategy_name = "Extension Family"; break;
                case Strategy::FolderCopy:      strategy_name = "Folder Copy";   break;
            }

            ImGui::Text("%s (%zu files)", strategy_name, static_cast<size_t>(group.files.size()));

            // Show action items.
            for (size_t i = 0; i < group.files.size(); ++i) {
                const auto& file = group.files[i];
                std::string path = PathUtils::wide_to_utf8(file.path);

                char buf[512];
                snprintf(buf, sizeof(buf), "%s", path.c_str());

                // Checkbox for selection.
                bool checked = (i == 0 || file.type == FileType::Duplicate);

                ImGui::Checkbox(checked ? "##check" : "##check");
                ImGui::SameLine();
                if (i == 0) {
                    ImGui::Text("%s", buf);
                } else {
                    std::string renamed = PathUtils::wide_to_utf8(file.path);
                    ImGui::Text("%s \xe2\x86\x92 %s", buf, renamed.c_str());
                }
            }

            // Action buttons.
            if (ImGui::Button("Apply All")) {
                auto actions = OrganizationSvc::generate_actions(group);
                OrganizationSvc::apply(actions);
            }

            ImGui::SameLine();
            if (ImGui::Button("Undo Last")) {
                OrganizationSvc::undo_actions();
            }

            ImGui::Unindent();
        }

        // Undo button at the bottom.
        if (ImGui::Button("Undo All Actions")) {
            OrganizationSvc::undo_all();
        }

        ImGui::TreePop();
    }
}

// Generate and preview actions for all duplicate groups.
void generate_preview_actions(const std::vector<DuplicateGroup>& groups) {
    ImGuiView::set_preview_state(OrganizationSvc::generate_actions(groups));
}

// Apply all previewed actions.
void apply_all_actions() {
    ImGuiView::apply_preview_actions();
}
