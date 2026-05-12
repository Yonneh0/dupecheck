#include <imgui.h>
#include "PreviewPanel.h"
#include "../organization/OrganizationSvc.h"

void render_preview_panel(const std::vector<DuplicateGroup>& groups) {
    if (ImGui::TreeNode("Preview")) {
        for (const auto& group : groups) {
            const char* sn = nullptr;
            switch (group.strategy) {
                case Strategy::ExactMatch:      sn = "Exact Match"; break;
                case Strategy::NameVariant:     sn = "Name Variant";  break;
                case Strategy::SizeHashSimilar: sn = "Size+Hash";     break;
                case Strategy::ExtensionFamily: sn = "Extension Family"; break;
                case Strategy::FolderCopy:      sn = "Folder Copy";   break;
            }

            if (ImGui::TreeNodeEx(&group, "%s (%zu files)", sn, static_cast<size_t>(group.files.size()))) {
                for (size_t i = 0; i < group.files.size(); ++i) {
                    std::string path = PathUtils::wide_to_utf8(group.files[i].path);
                    if (i > 0) ImGui::Indent();
                    ImGui::Text("%s", path.c_str());
                    if (i > 0) ImGui::Unindent();
                }

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

        if (ImGui::Button("Undo All Actions")) {
            while (!OrganizationSvc::history_.empty()) OrganizationSvc::undo_actions();
        }

        ImGui::TreePop();
    }
}