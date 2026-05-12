#include "ImGuiView.h"
#include <imgui.h>
#include <imgui_impl_win32.h>
#include "../database/DatabaseManager.h"
#include "../scanner/CachedScannerService.h"
#include "../hashing/HashEngine.h"
#include "../organization/OrganizationSvc.h"
#include "../utils/JsonConfig.h"
#include "Controls.h"

// Perform a full scan with the given path and update results + session.
static void perform_scan_impl(const wchar_t* path, DatabaseManager* db) {
    if (!path || path[0] == L'\0') return;

    CachedScannerService scanner(path);
    if (scanner.init()) {
        auto cached_files = scanner.scan(path);
        DuplicateEngine engine(ImGuiView::config_);
        std::vector<DuplicateGroup> result_groups = engine.find_duplicates(cached_files, ALL_STRATEGIES);
        ImGuiView::set_results(result_groups);

        // Save session to database.
        int64_t path_hash = 0;
        for (wchar_t c : path) {
            path_hash += static_cast<int64_t>(c);
        }
        if (db) {
            db->save_session(path_hash, static_cast<int>(cached_files.size()),
                             static_cast<int>(result_groups.size()), ALL_STRATEGIES);
        }
    }
}

inline constexpr uint32_t ALL_STRATEGIES = 0x1F;   // All five strategy bits set

void ImGuiView::perform_scan(const wchar_t* path) { perform_scan_impl(path, s_db_); }

std::vector<DuplicateGroup> ImGuiView::get_results() { return results_; }

std::wstring get_default_db_path() {
    wchar_t appdata[MAX_PATH];
    DWORD len = ExpandEnvironmentStringsW(L"%APPDATA%", appdata, ARRAYSIZE(appdata));
    std::wstring db_path;
    if (len > 0 && len < static_cast<DWORD>(ARRAYSIZE(appdata))) {
        db_path = std::wstring(appdata) + L"\\DupeCheck\\dupecheck.db";
    } else {
        const wchar_t* env = _wgetenv(L"APPDATA");
        db_path = (env ? std::wstring(env) : L"C:\\Windows") + L"\\DupeCheck\\dupecheck.db";
    }

    // Ensure the directory exists.
    auto dir_pos = db_path.find_last_of(L'\\');
    if (dir_pos != std::wstring::npos) {
        CreateDirectoryW(db_path.substr(0, dir_pos).c_str(), nullptr);
    }

    return db_path;
}

bool ImGuiView::init(HINSTANCE hInstance, int nCmdShow) {
    HICON hIcon = LoadIcon(hInstance, IDI_APPLICATION);
    HCURSOR hCursor = LoadCursor(nullptr, IDC_ARROW);

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = DefWindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = WND_CLASS_NAME;
    wc.hIcon = hIcon;
    wc.hCursor = hCursor;

    return RegisterClassEx(&wc) != 0;
}

static MSG g_msg{};

int run_gui(HINSTANCE hInstance, int nCmdShow, const std::wstring& default_path) {
    HWND hwnd = CreateWindowExW(0, WND_CLASS_NAME, L"DupeCheck",
                                WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                CW_USEDEFAULT, CW_USEDEFAULT, 1024, 768,
                                nullptr, nullptr, GetModuleHandle(nullptr), nullptr);

    if (!hwnd) return 1;

    std::wstring db_path = get_default_db_path();

    DatabaseManager db(db_path);
    if (!db.init()) {
        MessageBoxW(nullptr, L"Failed to initialize database.", L"Error", MB_ICONERROR);
        return 1;
    }

    s_db_ = &db;
    HashEngine::init_bcrypt();

    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui_ImplWin32_Init(hwnd);

    // Shared scan path — used by Controls.cpp.
    static std::wstring shared_scan_path = default_path.empty() ? L"" : default_path;

    bool done = false;
    while (!done) {
        if (PeekMessage(&g_msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&g_msg);
            DispatchMessage(&g_msg);
            if (g_msg.message == WM_QUIT) done = true;
        }

        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Render all controls in one place.
        render_controls(shared_scan_path);

        ImGui::Render();
        ImGui_ImplWin32_RenderDrawData(ImGui::GetDrawData());
    }

    ImGui_ImplWin32_Shutdown();
    HashEngine::cleanup();
    ImGui::DestroyContext();
    return static_cast<int>(g_msg.wParam);
}