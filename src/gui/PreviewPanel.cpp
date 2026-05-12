#include <imgui.h>
#include "PreviewPanel.h"
#include "ImGuiView.h"
#include "../organization/OrganizationSvc.h"
#include "../core/Strategy.h"

/// Confirmation dialog helper for destructive actions. Currently unused — reserved for future per-action confirmation UI.
[[maybe_unused]] static bool show_confirmation(const char* title, const char* message) {
    ImGui::OpenPopup(title);
    const ImVec2 popup_size(360, 180);
    ImGui::SetNextWindowSize(popup_size, ImGuiCond_Appearing);

    bool confirmed = false;
    if (ImGui::BeginPopupModal(title, nullptr, ImGuiWindowFlags_NoResize)) {
        ImGui::TextWrapped("%s", message);
        ImGui::Separator();
        ImVec2 button_size(100, 30);
        if (ImGui::Button("Confirm", button_size)) { confirmed = true; ImGui::CloseCurrentPopup(); }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", button_size)) { ImGui::CloseCurrentPopup(); }
        ImGui::EndPopup();
    }
    return confirmed;
}

void render_preview_panel(const std::vector<DuplicateGroup>& groups) {
    if (groups.empty()) {
        ImGui::TextDisabled("No duplicates found — try scanning a different folder.");
        // Show undo-all button only when there are actions to undo.
        if (!OrganizationSvc::history_.empty() && ImGui::Button("Undo All Actions")) {
            while (!OrganizationSvc::history_.empty()) OrganizationSvc::undo_actions();
        }
        return;
    }

    for (const auto& group : groups) {
        const char* sn = strategy_to_string(group.strategy);

        // Use the address as tree node ID to avoid collisions.
        if (ImGui::TreeNodeEx(&group, "%s (%zu files)", sn, static_cast<size_t>(group.files.size()))) {
            for (size_t i = 0; i < group.files.size(); ++i) {
                std::string path_str = PathUtils::wide_to_utf8(group.files[i].path);
                if (i > 0) ImGui::Indent();

                // Show the first file as "original" (green), rest as duplicates (yellow).
                if (i == 0) {
                    const ImVec4 green{0.6f, 1.0f, 0.6f, 1.0f};
                    ImGui::PushStyleColor(ImGuiCol_Text, green);
                    ImGui::Text("O %s", path_str.c_str());
                    ImGui::PopStyleColor();
                } else {
                    const ImVec4 yellow{1.0f, 0.95f, 0.6f, 1.0f};
                    ImGui::PushStyleColor(ImGuiCol_Text, yellow);
                    // Show the action that will be taken on this duplicate.
                    ImGui::Text("D %s (rename)", path_str.c_str());
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

    // Single undo-all button at the bottom — only shown when there are actions.
    if (!OrganizationSvc::history_.empty()) {
        if (ImGui::Button("Undo All Actions")) {
            while (!OrganizationSvc::history_.empty()) OrganizationSvc::undo_actions();
        }
    }
}