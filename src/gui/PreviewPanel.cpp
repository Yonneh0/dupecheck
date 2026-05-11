#include <imgui.h>
#include "../core/ActionModel.h"
#include "../organization/OrganizationSvc.h"

void render_preview_panel(const std::vector<DuplicateGroup>& groups) {
    if (ImGui::TreeNode("Preview")) {
        for (const auto& group : groups) {
            ImGui::Indent();
            const char* sn = nullptr;
            switch (group.strategy) {
                case Strategy::ExactMatch:      sn = "Exact Match"; break;
                case Strategy::NameVariant:     sn = "Name Variant";  break;
                case Strategy::SizeHashSimilar: sn = "Size+Hash";     break;
                case Strategy::ExtensionFamily: sn = "Extension Family"; break;
                case Strategy::FolderCopy:      sn = "Folder Copy";   break;
            }
            ImGui::Text("%s (%zu files)", sn, static_cast<size_t>(group.files.size()));

            for (size_t i = 0; i < group.files.size(); ++i) {
                std::string path = PathUtils::wide_to_utf8(group.files[i].path);
                bool checked = (i == 0 || group.files[i].type == FileType::Duplicate);
                ImGui::Checkbox(checked ? "##check" : "##check");
                ImGui::SameLine();
                if (i == 0) {
                    ImGui::Text("%s", path.c_str());
                } else {
                    std::string renamed = PathUtils::wide_to_utf8(OrganizationSvc::generate_renamed_path(group.files[i], static_cast<int>(i - 1)));
                    ImGui::Text("%s \xe2\x86\x92 %s", path.c_str(), renamed.c_str());
                }
            }

            if (ImGui::Button("Apply All")) { OrganizationSvc::apply(OrganizationSvc::generate_actions(group)); }
            ImGui::SameLine();
            if (ImGui::Button("Undo Last")) { OrganizationSvc::undo_actions(); }
            ImGui::Unindent();
        }
        if (ImGui::Button("Undo All Actions")) { while (!OrganizationSvc::history_.empty()) OrganizationSvc::undo_actions(); }
        ImGui::TreePop();
    }
}