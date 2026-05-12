#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include "../engine/DuplicateEngine.h"
#include "../database/DatabaseManager.h"

extern std::wstring g_db_path;

class ImGuiView {
public:
    static bool init(HINSTANCE hInstance, int nCmdShow);
    static void perform_scan(const wchar_t* path);
    static std::vector<DuplicateGroup> get_results();
    static void set_results(const std::vector<DuplicateGroup>& r) { results_ = r; }

private:
    inline static const wchar_t* WND_CLASS_NAME = L"DupeCheck";
    inline static DatabaseManager* s_db_ = nullptr;
    inline static StrategyConfig config_{3, 1024};
    inline static std::vector<DuplicateGroup> results_;
};

int run_gui(HINSTANCE hInstance, int nCmdShow, const std::wstring& default_path);