#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <imgui.h>
#include "SettingsDialog.h"
#include "../utils/JsonConfig.h"
#include "../service/ServiceHost.h"

void render_settings_dialog() {
    if (!ImGui::Button("Settings")) return;
    ImGui::OpenPopup("##settings");
    const ImVec2 popup_size(400, 520);
    ImGui::SetNextWindowSize(popup_size, ImGuiCond_Appearing);

    StrategyConfig& cfg = get_strategy_config_impl();

    int orig_threshold = cfg.name_similarity_threshold;
    uint32_t orig_tolerance = cfg.hash_tolerance;
    SYSTEM_INFO info{};
    GetSystemInfo(&info);
    DWORD proc_count = info.dwNumberOfProcessors;
    int max_processors = static_cast<int>((proc_count > 1) ? (proc_count - 1) : 1);

    std::wstring db_dir = get_default_db_path();
    auto dir_pos = db_dir.find_last_of(L'\\');
    std::wstring path = (dir_pos != std::wstring::npos ? db_dir.substr(0, dir_pos) : db_dir) + L"\\settings.json";
    auto loaded = JsonConfig::load(path);

    int threshold = cfg.name_similarity_threshold;
    int tolerance_int = static_cast<int>(cfg.hash_tolerance);
    int hasher_count = max_processors;

    auto load_val = [&](const char* key, int& out) {
        auto it = loaded.find(key);
        if (it != loaded.end()) {
            try { out = std::stoi(it->second); } catch (...) {}
        }
    };
    load_val("name_similarity_threshold", threshold);
    load_val("hash_tolerance", tolerance_int);
    load_val("max_concurrent_hashers", hasher_count);

    if (ImGui::BeginPopupModal("##settings", nullptr, 0)) {
        ImGui::Text("Settings");
        ImGui::Separator();
        ImGui::SetNextItemWidth(280);
        ImGui::SliderInt("Name Similarity Threshold (0-10)", &threshold, 0, 10);
        ImGui::SetNextItemWidth(280);
        ImGui::SliderInt("Hash Tolerance (bytes)", &tolerance_int, 256, 4096);
        ImGui::SetNextItemWidth(280);
        ImGui::SliderInt("Max Concurrent Hashers", &hasher_count, 1, max_processors);

        if (ImGui::Button("Save Settings")) {
            if (threshold < 0 || threshold > 10) {
                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.6f, 1.0f), "Threshold must be between 0 and 10");
            } else if (tolerance_int < 256 || tolerance_int > 4096) {
                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.6f, 1.0f), "Tolerance must be between 256 and 4096");
            } else if (hasher_count < 1 || hasher_count > max_processors) {
                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.6f, 1.0f), "Hashers must be between 1 and %d", max_processors);
            } else {
                cfg.name_similarity_threshold = threshold;
                cfg.hash_tolerance = static_cast<uint32_t>(tolerance_int);

                std::unordered_map<std::string, std::string> config_data = {
                    {"name_similarity_threshold", std::to_string(threshold)},
                    {"hash_tolerance", std::to_string(tolerance_int)},
                    {"max_concurrent_hashers", std::to_string(hasher_count)}
                };

                JsonConfig::save(path, config_data);
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            cfg.name_similarity_threshold = orig_threshold;
            cfg.hash_tolerance = orig_tolerance;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}