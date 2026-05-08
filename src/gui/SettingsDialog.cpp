#include <windows.h>
#include <imgui.h>
#include <unordered_map>
#include "../utils/JsonConfig.h"
#include "SettingsDialog.h"
#include "ImGuiView.h"

StrategyConfig& get_strategy_config() {
    return g_config;
}

std::wstring g_scan_path;

// Settings dialog (modal).
void render_settings_dialog() {
    if (ImGui::Button("Settings")) {
        ImGui::OpenPopup("##settings");
    }

    ImGui::SetNextWindowPos(ImVec2(400, 300), ImGuiCond_Appearing);
    ImGui::SetNextWindowSize(ImVec2(400, 500));

    if (ImGui::BeginPopupModal("##settings", nullptr, 0)) {
        // Thresholds — read from the module-level config.
        StrategyConfig& cfg = get_strategy_config();
        int threshold = static_cast<int>(cfg.name_similarity_threshold);
        uint32_t tolerance = cfg.hash_tolerance;

        ImGui::SliderInt("Name Similarity Threshold", &threshold, 0, 10);
        ImGui::SliderInt("Hash Tolerance (bytes)", static_cast<int*>(&tolerance), 256, 4096);

        // Max concurrent hashers.
        SYSTEM_INFO info;
        GetSystemInfo(&info);
        int max_hashers = std::max(1, static_cast<int>(info.dwNumberOfProcessors) - 1);
        int hasher_count = (int)(max_hashers / 2);
        ImGui::SliderInt("Max Concurrent Hashers", &hasher_count, 1, max_hashers);

        // Enable service.
        bool enable_service = cfg.service_enabled;
        if (ImGui::Checkbox("Enable Windows Service", &enable_service)) {
            cfg.service_enabled = enable_service;
        }

        // Extension families editor.
        ImGui::Text("Extension Families:");
        if (ImGui::Button("+ Add Family")) {
            // Add new family UI...
        }

        // Save button.
        static wchar_t path_buf[512];
        wsprintfW(path_buf, L"%s", g_scan_path.c_str());
        std::wstring appdata;
        const wchar_t* env = _wgetenv(L"APPDATA");
        if (env) {
            appdata = std::wstring(env);
        } else {
            appdata = L"C:\\Windows";
        }

        std::wstring settings_path = appdata + L"\\DupeCheck\\settings.json";

        if (ImGui::Button("Save Settings")) {
            // Save config to file.
            cfg.name_similarity_threshold = threshold;
            cfg.hash_tolerance = tolerance;

            std::unordered_map<std::string, std::string> config_data = {
                {"name_similarity_threshold", std::to_string(threshold)},
                {"hash_tolerance", std::to_string(tolerance)},
                {"max_concurrent_hashers", std::to_string(hasher_count)}
            };

            // Save the scan path separately.
            config_data["scan_path"] = PathUtils::wide_to_utf8(g_scan_path);

            JsonConfig::save(settings_path, config_data);
        }

        ImGui::EndPopup();
    }
}
