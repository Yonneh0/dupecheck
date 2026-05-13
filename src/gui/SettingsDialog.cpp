#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <imgui.h>
#include "SettingsDialog.h"
#include "../utils/JsonConfig.h"
#include "../utils/DbPath.h"

inline int get_max_processors() {
    SYSTEM_INFO info{};
    GetSystemInfo(&info);
    return static_cast<int>((info.dwNumberOfProcessors > 1) ? (info.dwNumberOfProcessors - 1) : 1);
}

void render_settings_dialog() {
    if (!ImGui::Button("Settings")) return;
    if (!ImGui::IsPopupOpen("##settings")) ImGui::OpenPopup("##settings");

    const ImVec2 popup_size(400, 520);
    ImGui::SetNextWindowSize(popup_size, ImGuiCond_Appearing);

    StrategyConfig& cfg = get_strategy_config();

    int threshold  = cfg.name_similarity_threshold;
    int tolerance  = static_cast<int>(cfg.hash_tolerance);
    int max_procs  = get_max_processors();
    int hasher_count = max_procs;

    // Capture before loading persisted settings.
    const int orig_threshold = threshold;
    const uint32_t orig_tolerance = cfg.hash_tolerance;

    std::wstring db_path   = get_default_db_path();
    auto dir_pos           = db_path.find_last_of(L'\\');
    std::wstring config_file = (dir_pos != std::wstring::npos) ? db_path.substr(0, dir_pos) : db_path;
    config_file += L"\\settings.json";

    const auto loaded = JsonConfig::load(config_file);

    if (auto it = loaded.find("name_similarity_threshold"); it != loaded.end()) threshold = std::stoi(it->second);
    if (auto it = loaded.find("hash_tolerance"); it != loaded.end()) tolerance = std::stoi(it->second);
    if (auto it = loaded.find("max_concurrent_hashers"); it != loaded.end()) hasher_count = std::stoi(it->second);

    if (ImGui::BeginPopupModal("##settings", nullptr, 0)) {
        ImGui::Text("Settings");
        ImGui::Separator();
        ImGui::SetNextItemWidth(280);
        ImGui::SliderInt("Name Similarity Threshold (0-10)", &threshold, 0, 10);
        ImGui::SetNextItemWidth(280);
        ImGui::SliderInt("Hash Tolerance (bytes)", &tolerance, 256, 4096);
        ImGui::SetNextItemWidth(280);
        ImGui::SliderInt("Max Concurrent Hashers", &hasher_count, 1, max_procs);

        if (ImGui::Button("Save Settings")) {
            bool valid = true;
            if (threshold < 0 || threshold > 10) {
                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.6f, 1.0f), "Threshold must be between 0 and 10"); valid = false;
            } else if (tolerance < 256 || tolerance > 4096) {
                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.6f, 1.0f), "Tolerance must be between 256 and 4096"); valid = false;
            } else if (hasher_count < 1 || hasher_count > max_procs) {
                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.6f, 1.0f), "Hashers must be between 1 and %d", max_procs); valid = false;
            }

            if (valid) {
                cfg.name_similarity_threshold = threshold;
                cfg.hash_tolerance              = static_cast<uint32_t>(tolerance);

                std::unordered_map<std::string, std::string> config_data = {
                    {"name_similarity_threshold", std::to_string(threshold)},
                    {"hash_tolerance",            std::to_string(tolerance)},
                    {"max_concurrent_hashers",    std::to_string(hasher_count)}
                };

                JsonConfig::save(config_file, config_data);
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            threshold  = orig_threshold;
            tolerance  = static_cast<int>(orig_tolerance);
            hasher_count = max_procs;
            cfg.name_similarity_threshold = orig_threshold;
            cfg.hash_tolerance              = orig_tolerance;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}
