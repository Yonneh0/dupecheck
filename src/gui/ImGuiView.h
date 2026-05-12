#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include "../engine/DuplicateEngine.h"
#include "../database/DatabaseManager.h"

/// Path to the SQLite database (set during init, used by scan).
extern std::wstring g_db_path;

class ImGuiView {
public:
    /// Initialize the window class and register the GUI.
    static bool init(HINSTANCE hInstance, int nCmdShow);

    /// Perform a full scan with the given path and update results + session in the database.
    static void perform_scan(const wchar_t* path);

    /// Get the current scan results from any part of the UI.
    static std::vector<DuplicateGroup> get_results();

    // Set results directly (called by Controls.cpp).
    static void set_results(const std::vector<DuplicateGroup>& r) { results_ = r; }

private:
    inline static const wchar_t* WND_CLASS_NAME = L"DupeCheck";
    inline static DatabaseManager* s_db_ = nullptr;
    inline static StrategyConfig config_{3, 1024};
    inline static std::vector<DuplicateGroup> results_;

    // Internal scan implementation that also sets progress.
    static void run_scan_impl(const wchar_t* path);
};

/// Entry point for the GUI — called when no CLI service mode is active.
int run_gui(HINSTANCE hInstance, int nCmdShow, const std::wstring& default_path);