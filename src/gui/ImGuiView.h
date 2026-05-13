#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include "../core/Strategy.h"
#include "../database/DatabaseManager.h"
#include "../engine/DuplicateEngine.h"
#include "../utils/DbPath.h"

inline std::wstring g_db_path = get_default_db_path();
constexpr const wchar_t* WND_CLASS_NAME = L"DupeCheckWindow";

class ImGuiView {
public:
    static bool init(HINSTANCE hInstance, int nCmdShow);
    static void perform_scan(const wchar_t* path);
    static std::vector<DuplicateGroup> get_results();
    static void set_results(const std::vector<DuplicateGroup>& r) { results_ = r; }
    static StrategyConfig& config() noexcept { return s_config_; }
    static void set_db(DatabaseManager* db) { s_db_ = db; }

private:
    inline static DatabaseManager* s_db_   = nullptr;
    inline static StrategyConfig s_config_{};
    inline static std::vector<DuplicateGroup> results_;
};

int run_gui(HINSTANCE hInstance, int nCmdShow, const std::wstring& default_path);