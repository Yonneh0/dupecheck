#include <imgui.h>
#include "PreviewPanel.h"
#include "../organization/OrganizationSvc.h"
#include "../core/Strategy.h"

void render_preview_panel(const std::vector<DuplicateGroup>& groups) {
    if (groups.empty()) {
        ImGui::TextDisabled("No duplicates found — try scanning a different folder.");
        return;
    }

    for (const auto& group : groups) {
        const char* sn = strategy_to_string(group.strategy);

        // Use the address as tree node ID to avoid collisions.
        if (ImGui::TreeNodeEx(&group, "%s (%zu files)", sn, static_cast<size_t>(group.files.size()))) {
            for (size_t i = 0; i < group.files.size(); ++i) {
                std::string path = PathUtils::wide_to_utf8(group.files[i].path);
                if (i > 0) ImGui::Indent();

                // Show the first file as "original", rest as duplicates.
                if (i == 0) {
                    const ImVec4 green{0.6f, 1.0f, 0.6f, 1.0f};
                    ImGui::PushStyleColor(ImGuiCol_Text, green);
                    ImGui::Text("O %s", path.c_str());
                    ImGui::PopStyleColor();
                } else {
                    const ImVec4 yellow{1.0f, 0.95f, 0.6f, 1.0f};
                    ImGui::PushStyleColor(ImGuiCol_Text, yellow);
                    ImGui::Text("D %s", path.c_str());
                    ImGui::PopStyleColor();
                }

                if (i > 0) ImGui::Unindent();
            }

            // Action buttons per group.
            if (ImGui::Button("Apply All")) {
                auto items = OrganizationSvc::generate_actions(group, ActionType::Rename);
                OrganizationSvc::apply(items);
            }
            ImGui::SameLine();
            if (ImGui::Button("Undo Last")) {
                OrganizationSvc::undo_actions();
            }

            ImGui::TreePop();
        }
    }

    // Undo all actions button at bottom.
    if (!OrganizationSvc::history_.empty()) {
        if (ImGui::Button("Undo All Actions")) {
            while (!OrganizationSvc::history_.empty()) OrganizationSvc::undo_actions();
        }
    }
}