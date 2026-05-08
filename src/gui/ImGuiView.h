#pragma once
#include <windows.h>
#include <vector>
#include <string>
#include <any>
#include "../database/DatabaseManager.h"
#include "../engine/DuplicateEngine.h"

class ImGuiView {
public:
    // Initialize the GUI.
    static bool init(HINSTANCE hInstance, int nCmdShow);

    // Run the main window loop.
    static void run();

    // Start a new scan session.
    static void start_scan(const wchar_t* path);

    // Get current results.
    static std::vector<DuplicateGroup> get_results();

    // Set the last-generated preview action items.
    static void set_preview_state(const std::vector<ActionItem>& actions);

    // Apply previewed actions (for PreviewPanel integration).
    static void apply_preview_actions();

private:
    static const wchar_t* WND_CLASS_NAME;
};

// Main entry point for ImGui GUI mode.
int run_gui(HINSTANCE hInstance, int nCmdShow,
            const std::wstring& default_path, DatabaseManager& db);