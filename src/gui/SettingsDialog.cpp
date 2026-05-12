#include <windows.h>
#include <imgui.h>
#include "SettingsDialog.h"
#include "../utils/JsonConfig.h"
#include "../database/DatabaseManager.h"

static StrategyConfig& get_strategy_config_impl() {
    static StrategyConfig config{3, 1024};
    return config;
}

void render_settings_dialog() {
    if (!ImGui::Button("Settings")) return;
    ImGui::OpenPopup("##settings");
    const ImVec2 popup_size(400, 520);
    ImGui::SetNextWindowSize(popup_size, ImGuiCond_Appearing);

    StrategyConfig& cfg = get_strategy_config_impl();

    // Save original values so Cancel restores them correctly.
    int orig_threshold = cfg.name_similarity_threshold;
    uint32_t orig_tolerance = cfg.hash_tolerance;
    SYSTEM_INFO info{};
    GetSystemInfo(&info);
    int orig_hasher_count = static_cast<int>(std::max(1, static_cast<int>(info.dwNumberOfProcessors) - 1));

    // Load current settings from disk so the dialog is pre-populated.
    const wchar_t* env = _wgetenv(L"APPDATA");
    std::wstring path = (env ? std::wstring(env) : L"C:\\Windows") + L"\\DupeCheck\\settings.json";
    auto loaded = JsonConfig::load(path);

    int threshold = cfg.name_similarity_threshold;
    uint32_t tolerance = cfg.hash_tolerance;
    int hasher_count = orig_hasher_count;

    // Restore from disk if present.
    auto load_val = [&](const char* key, int& out) {
        auto it = loaded.find(key);
        if (it != loaded.end()) {
            try { out = std::stoi(it->second); } catch (...) {}
        }
    };
    load_val("name_similarity_threshold", threshold);
    load_val("hash_tolerance", static_cast<int&>(tolerance));
    load_val("max_concurrent_hashers", hasher_count);

    if (ImGui::BeginPopupModal("##settings", nullptr, 0)) {
        ImGui::Text("Settings");
        ImGui::Separator();
        ImGui::SetNextItemWidth(280);
        ImGui::SliderInt("Name Similarity Threshold", &threshold, 0, 10);
        ImGui::SetNextItemWidth(280);
        ImGui::SliderInt("Hash Tolerance (bytes)", static_cast<int*>(&tolerance), 256, 4096);
        ImGui::SetNextItemWidth(280);
        ImGui::SliderInt("Max Concurrent Hashers", &hasher_count, 1, std::max(1, info.dwNumberOfProcessors - 1));

        if (ImGui::Button("Save Settings")) {
            cfg.name_similarity_threshold = threshold;
            cfg.hash_tolerance = tolerance;

            std::unordered_map<std::string, std::string> config_data = {
                {"name_similarity_threshold", std::to_string(threshold)},
                {"hash_tolerance", std::to_string(tolerance)},
                {"max_concurrent_hashers", std::to_string(hasher_count)}
            };

            JsonConfig::save(path, config_data);
            ImGui::CloseCurrentPopup();
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
