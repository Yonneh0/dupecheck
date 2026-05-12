#include <imgui.h>
#include "PreviewPanel.h"
#include "ImGuiView.h"
#include "../organization/OrganizationSvc.h"
#include "../core/Strategy.h"

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
        // Color-code strategy type.
        ImVec4 strategy_color{};
        switch (group.strategy) {
            case Strategy::ExactMatch:      strategy_color = ImVec4(0.6f, 1.0f, 0.6f, 1.0f); break;
            case Strategy::NameVariant:     strategy_color = ImVec4(0.7f, 0.85f, 1.0f, 1.0f); break;
            case Strategy::SizeHashSimilar: strategy_color = ImVec4(1.0f, 0.85f, 0.6f, 1.0f); break;
            case Strategy::ExtensionFamily: strategy_color = ImVec4(0.9f, 0.7f, 1.0f, 1.0f); break;
            case Strategy::FolderCopy:      strategy_color = ImVec4(1.0f, 1.0f, 0.6f, 1.0f); break;
        }

        if (ImGui::TreeNodeEx(&group, "%s (%zu files)", group.label.c_str(), static_cast<size_t>(group.files.size()))) {
            for (size_t i = 0; i < group.files.size(); ++i) {
                std::string path_str = PathUtils::wide_to_utf8(group.files[i].path);

                if (i > 0) ImGui::Indent();

                // First file is the original (green), rest are duplicates (yellow).
                if (i == 0) {
                    ImGui::PushStyleColor(ImGuiCol_Text, strategy_color);
                    ImGui::Text("O %s", path_str.c_str());
                } else {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.95f, 0.6f, 1.0f));
                    ImGui::Text("D %s", path_str.c_str());
                }

                // Show file size below the path.
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
                std::string size_str = (group.files[i].size >= 1024 * 1024)
                    ? std::to_string(group.files[i].size / (1024 * 1024)) + " MB"
                    : (group.files[i].size >= 1024)
                        ? std::to_string(group.files[i].size / 1024) + " KB"
                        : std::to_string(group.files[i].size) + " B";
                ImGui::Text("   %s", size_str.c_str());
                ImGui::PopStyleColor();

                if (i > 0) ImGui::Unindent();
                ImGui::PopStyleColor();
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
