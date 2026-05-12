#include <windows.h>
#include <imgui.h>
#include "SettingsDialog.h"
#include "../utils/JsonConfig.h"

// Global scan path variable used throughout the app.
std::wstring g_scan_path;

StrategyConfig& get_strategy_config() {
    static StrategyConfig config{3, 1024};
    return config;
}

void render_settings_dialog() {
    if (ImGui::Button("Settings")) ImGui::OpenPopup("##settings");
    ImGui::SetNextWindowPos(ImVec2(400, 300), ImGuiCond_Appearing);
    ImGui::SetNextWindowSize(ImVec2(400, 500));

    if (ImGui::BeginPopupModal("##settings", nullptr, 0)) {
        StrategyConfig& cfg = get_strategy_config();
        int threshold = static_cast<int>(cfg.name_similarity_threshold);
        uint32_t tolerance = cfg.hash_tolerance;
        ImGui::SliderInt("Name Similarity Threshold", &threshold, 0, 10);
        ImGui::SliderInt("Hash Tolerance (bytes)", static_cast<int*>(&tolerance), 256, 4096);

        SYSTEM_INFO info{}; GetSystemInfo(&info);
        int hasher_count = static_cast<int>(std::max(1, static_cast<int>(info.dwNumberOfProcessors) - 1));
        ImGui::SliderInt("Max Concurrent Hashers", &hasher_count, 1, std::max(1, info.dwNumberOfProcessors - 1));

        if (ImGui::Button("+ Add Family")) { /* TODO: add extension family */ }
        if (ImGui::Button("Save Settings")) {
            cfg.name_similarity_threshold = threshold;
            cfg.hash_tolerance = tolerance;

            std::unordered_map<std::string, std::string> config_data = {
                {"name_similarity_threshold", std::to_string(threshold)},
                {"hash_tolerance", std::to_string(tolerance)},
                {"max_concurrent_hashers", std::to_string(hasher_count)}
            };

            const wchar_t* env = _wgetenv(L"APPDATA");
            std::wstring path = (env ? std::wstring(env) : L"C:\\Windows") + L"\\DupeCheck\\settings.json";
            JsonConfig::save(path, config_data);
        }
        ImGui::EndPopup();
    }
}