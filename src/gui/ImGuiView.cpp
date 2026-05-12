#include "ImGuiView.h"
#include <imgui.h>
#include <imgui_impl_win32.h>
#include "../database/DatabaseManager.h"
#include "../scanner/CachedScannerService.h"
#include "../hashing/HashEngine.h"

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

    bool done = false;
    while (!done) {
        if (PeekMessage(&g_msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&g_msg);
            DispatchMessage(&g_msg);
            if (g_msg.message == WM_QUIT) done = true;
        }

        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        static wchar_t path_buf[512] = L"";
        if (!default_path.empty() && wcslen(path_buf) == 0) {
            wcscpy(path_buf, default_path.c_str());
        }

        ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("DupeCheck")) {
            if (ImGui::InputTextW(L"##path", path_buf, ARRAYSIZE(path_buf))) {
                CachedScannerService scanner(path_buf);
                if (scanner.init()) {
                    auto cached_files = scanner.scan(path_buf);
                    DuplicateEngine engine(config_);
                    results_ = engine.find_duplicates(cached_files, 0x1F);

                    int64_t path_hash = 0;
                    for (wchar_t c : path_buf) {
                        path_hash += static_cast<int64_t>(c);
                    }
                    db.save_session(path_hash, static_cast<int>(cached_files.size()),
                                    static_cast<int>(results_.size()), 0x1F);
                }
            }

            if (ImGui::Button("Scan", ImVec2(100, 30))) {
                CachedScannerService scanner(path_buf);
                if (scanner.init()) {
                    auto cached_files = scanner.scan(path_buf);
                    DuplicateEngine engine(config_);
                    results_ = engine.find_duplicates(cached_files, 0x1F);
                }
            }

            for (const auto& group : results_) {
                bool open = ImGui::TreeNodeEx(
                    static_cast<const void*>(&group),
                    "%s (%zu files)",
                    strategy_to_string(group.strategy).c_str(),
                    static_cast<size_t>(group.files.size()));

                if (open) {
                    for (const auto& file : group.files) {
                        std::string path = PathUtils::wide_to_utf8(file.path);
                        ImGui::Text("  %s", path.c_str());
                    }
                    ImGui::TreePop();
                }
            }

            if (ImGui::Button("Apply All")) {
                auto items = OrganizationSvc::generate_actions(results_, ActionType::Rename);
                OrganizationSvc::apply(items);
            }
            ImGui::SameLine();
            if (ImGui::Button("Undo Last")) {
                OrganizationSvc::undo_actions();
            }

            ImGui::End();
        }

        ImGui::Render();
        ImGui_ImplWin32_RenderDrawData(ImGui::GetDrawData());
    }

    ImGui_ImplWin32_Shutdown();
    HashEngine::cleanup();
    ImGui::DestroyContext();
    return static_cast<int>(g_msg.wParam);
}
