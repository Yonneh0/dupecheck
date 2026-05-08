#include "ImGuiView.h"
#include <imgui.h>
#include <imgui_impl_win32.h>
#include "../hashing/HashEngine.h"
#include "../scanner/FileScanner.h"
#include "../engine/DuplicateEngine.h"
#include "../organization/OrganizationSvc.h"

const wchar_t* ImGuiView::WND_CLASS_NAME = L"DupeCheck";

// Module-level state.
static std::vector<DuplicateGroup> s_results;
static DatabaseManager* s_db = nullptr;
StrategyConfig g_config{3, 1024};

bool ImGuiView::init(HINSTANCE hInstance, int nCmdShow) {
    // Register window class with icon and cursor.
    HICON hIcon = LoadIcon(hInstance, IDI_APPLICATION);
    HCURSOR hCursor = LoadCursor(nullptr, IDC_ARROW);

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = DefWindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = WND_CLASS_NAME;
    wc.hIcon = hIcon;
    wc.hCursor = hCursor;

    ATOM atom = RegisterClassEx(&wc);
    return atom != 0;
}

void ImGuiView::run() {
    // Create main window.
    HWND hwnd = CreateWindow(WND_CLASS_NAME, L"DupeCheck",
                            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                            CW_USEDEFAULT, CW_USEDEFAULT, 1024, 768,
                            nullptr, nullptr, GetModuleHandle(nullptr), nullptr);

    if (!hwnd) return;

    MSG msg = {};
    bool done = false;

    // Initialize ImGui.
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    auto& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplWin32_Init(hwnd);

    // Main rendering loop — render continuously using a direct D2D or GDI backend.
    while (!done) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (msg.message == WM_QUIT) done = true;
        }

        // Render frame.
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Main window layout.
        ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("DupeCheck")) {
            static wchar_t path_buf[512] = L"";

            // Path input + scan button.
            if (ImGui::InputText("##path", reinterpret_cast<char*>(path_buf), sizeof(path_buf) / sizeof(wchar_t))) {
                start_scan(path_buf);
            }

            // Strategy panels.
            for (auto& group : s_results) {
                bool open = ImGui::TreeNodeEx(
                    &group,
                    "%s (%zu files)",
                    strategy_to_string(group.strategy).c_str(),
                    static_cast<size_t>(group.files.size()));

                if (open) {
                    // Preview panel.
                    for (auto& file : group.files) {
                        std::string path = PathUtils::wide_to_utf8(file.path);
                        ImGui::Text("  %s", path.c_str());
                    }
                    ImGui::TreePop();
                }
            }

            // Bottom bar.
            if (ImGui::Button("Apply All")) {
                auto actions = OrganizationSvc::generate_actions(s_results);
                OrganizationSvc::apply(actions);
            }

            ImGui::SameLine();
            if (ImGui::Button("Undo Last")) {
                OrganizationSvc::undo_actions();
            }

            // Settings button.
            ImGui::SameLine(500);
            if (ImGui::Button("Settings")) {
                // Open settings dialog in a real implementation.
            }

            ImGui::End();
        }

        // Render ImGui to screen via GDI backend.
        ImGui::Render();
        ImGui_ImplWin32_RenderDrawData(ImGui::GetDrawData());
    }

    // Cleanup.
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiView::start_scan(const wchar_t* path) {
    FileScanner scanner([path](uint64_t total, uint64_t processed) {
        // Could update progress bar here.
        (void)total;
        (void)processed;
    });

    auto files = scanner.scan(path);

    DuplicateEngine engine(g_config);
    s_results = engine.find_duplicates(files,
        static_cast<uint32_t>(Strategy::ExactMatch) |
        static_cast<uint32_t>(Strategy::NameVariant) |
        static_cast<uint32_t>(Strategy::SizeHashSimilar) |
        static_cast<uint32_t>(Strategy::ExtensionFamily));

    // Update database session if available.
    if (s_db) {
        int64_t path_hash = 0;
        for (char c : PathUtils::wide_to_utf8(path)) {
            path_hash += static_cast<int64_t>(c);
        }
        s_db->save_session(path_hash, static_cast<int>(files.size()),
                           static_cast<int>(s_results.size()),
                           Strategy::ExactMatch | Strategy::NameVariant |
                           Strategy::SizeHashSimilar | Strategy::ExtensionFamily);
    }
}

std::vector<DuplicateGroup> ImGuiView::get_results() {
    return s_results;
}

void ImGuiView::apply_preview_actions() {
    // Apply previewed actions (re-apply the current results).
    auto actions = OrganizationSvc::generate_actions(s_results);
    OrganizationSvc::apply(actions);
}

int run_gui(HINSTANCE hInstance, int nCmdShow,
            const std::wstring& default_path, DatabaseManager& db) {

    if (!ImGuiView::init(hInstance, nCmdShow)) {
        MessageBoxW(nullptr, L"Failed to initialize window.", L"Error", MB_ICONERROR);
        return 1;
    }

    s_db = &db;

    // Use the module-level config so settings changes propagate.
    g_config.service_enabled = true;

    ImGuiView::start_scan(default_path.c_str());
    ImGuiView::run();

    return 0;
}