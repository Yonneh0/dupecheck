#include <windows.h>
#include <imgui.h>
#include "SettingsDialog.h"
#include "../utils/JsonConfig.h"

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
    int max_processors = static_cast<int>(std::max(1u, info.dwNumberOfProcessors - 1));

    // Load current settings from disk so the dialog is pre-populated.
    const wchar_t* env = _wgetenv(L"APPDATA");
    std::wstring path = (env ? std::wstring(env) : L"C:\\Windows") + L"\\DupeCheck\\settings.json";
    auto loaded = JsonConfig::load(path);

    int threshold = cfg.name_similarity_threshold;
    uint32_t tolerance = cfg.hash_tolerance;
    int hasher_count = max_processors;

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
        ImGui::SliderInt("Name Similarity Threshold (0-10)", &threshold, 0, 10);
        ImGui::SetNextItemWidth(280);
        ImGui::SliderInt("Hash Tolerance (bytes)", static_cast<int*>(&tolerance), 256, 4096);
        ImGui::SetNextItemWidth(280);
        ImGui::SliderInt("Max Concurrent Hashers", &hasher_count, 1, max_processors);

        if (ImGui::Button("Save Settings")) {
            // Validate: threshold must be non-negative, tolerance at least 256.
            if (threshold < 0 || threshold > 10) {
                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.6f, 1.0f), "Threshold must be between 0 and 10");
            } else if (tolerance < 256 || tolerance > 4096) {
                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.6f, 1.0f), "Tolerance must be between 256 and 4096");
            } else if (hasher_count < 1 || hasher_count > max_processors) {
                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.6f, 1.0f), "Hashers must be between 1 and %d", max_processors);
            } else {
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
