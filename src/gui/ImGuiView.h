#pragma once
#include <windows.h>
#include <vector>
#include "../engine/DuplicateEngine.h"
#include "../database/DatabaseManager.h"

// Main ImGui-based GUI for the duplicate finder application.
class ImGuiView {
public:
    static bool init(HINSTANCE hInstance, int nCmdShow);
    static void run();
    static std::vector<DuplicateGroup> get_results() { return results_; }

private:
    inline static const wchar_t* WND_CLASS_NAME = L"DupeCheck";
    inline static DatabaseManager* s_db_ = nullptr;
    inline static StrategyConfig config_{3, 1024};
    inline static std::vector<DuplicateGroup> results_;
};

int run_gui(HINSTANCE hInstance, int nCmdShow, const std::wstring& default_path);