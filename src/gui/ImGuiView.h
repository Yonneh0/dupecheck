#pragma once
#include <windows.h>
#include <vector>
#include <string>
#include <any>
#include "../database/DatabaseManager.h"
#include "../engine/DuplicateEngine.h"

class ImGuiView {
public:
    static bool init(HINSTANCE hInstance, int nCmdShow);

    static void run();

    static void start_scan(const wchar_t* path);

    static std::vector<DuplicateGroup> get_results();

    static void set_preview_state(const std::vector<ActionItem>& actions);

    static void apply_preview_actions();

private:
    static const wchar_t* WND_CLASS_NAME;
};

int run_gui(HINSTANCE hInstance, int nCmdShow,
            const std::wstring& default_path, DatabaseManager& db);
