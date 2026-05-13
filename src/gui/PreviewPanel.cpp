#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <imgui.h>
#include "PreviewPanel.h"
#include "ImGuiView.h"
#include "../organization/OrganizationSvc.h"
#include "../core/Strategy.h"

void render_preview_panel(const std::vector<DuplicateGroup>& groups) {
    if (groups.empty()) {
        ImGui::TextDisabled("No duplicates found — try scanning a different folder.");
        return;
    }

    for (const auto& group : groups) {
        ImVec4 strategy_color{};
        switch (group.strategy) {
            case Strategy::ExactMatch:      strategy_color = ImVec4(0.6f, 1.0f, 0.6f, 1.0f); break;
            case Strategy::NameVariant:     strategy_color = ImVec4(0.7f, 0.85f, 1.0f, 1.0f); break;
            case Strategy::SizeHashSimilar: strategy_color = ImVec4(1.0f, 0.85f, 0.6f, 1.0f); break;
            case Strategy::ExtensionFamily: strategy_color = ImVec4(0.9f, 0.7f, 1.0f, 1.0f); break;
            case Strategy::FolderCopy:      strategy_color = ImVec4(1.0f, 1.0f, 0.6f, 1.0f); break;
        }

        std::string tree_id = "group_" + std::to_string(reinterpret_cast<uintptr_t>(&group));
        if (ImGui::TreeNodeEx(tree_id.c_str(), ImGuiTreeNodeFlags_None, "%s (%zu files)", group.label.c_str(), static_cast<size_t>(group.files.size()))) {
            for (size_t i = 0; i < group.files.size(); ++i) {
                std::string path_str = PathUtils::wide_to_utf8(group.files[i].path);

                if (i > 0) ImGui::Indent();

                if (i == 0) {
                    ImGui::PushStyleColor(ImGuiCol_Text, strategy_color);
                    ImGui::Text("O %s", path_str.c_str());
                } else {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.95f, 0.6f, 1.0f));
                    ImGui::Text("D %s", path_str.c_str());
                }

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

            if (ImGui::Button("Apply All")) {
                auto items = OrganizationSvc::generate_actions(group, ActionType::Rename);
                OrganizationSvc::apply(items);
            }
            ImGui::SameLine();
            if (ImGui::Button("Undo")) {
                OrganizationSvc::undo_actions(1);
            }

            ImGui::TreePop();
        }
    }

    auto& history = OrganizationSvc::get_history();
    if (!history.empty() && ImGui::Button("Undo All Actions")) {
        OrganizationSvc::undo_actions(static_cast<int>(history.size()));
    }
}