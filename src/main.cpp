#include <windows.h>
#include "database/DatabaseManager.h"
#include "gui/ImGuiView.h"
#include "hashing/HashEngine.h"
#include "service/ServiceHost.h"

// Entry point: either runs the GUI or a CLI service.
int main() {
    HashEngine::init_bcrypt();

    ServiceArgs args = parse_args(__argc, __argv);

    switch (args.command) {
        case CliCommand::InstallService: {
            wchar_t exe_path[MAX_PATH];
            GetModuleFileNameW(nullptr, exe_path, ARRAYSIZE(exe_path));
            install_service(exe_path, L"C:\\");
            MessageBoxA(nullptr, "Service installed successfully.", "DupeCheck", MB_ICONINFORMATION);
            break;
        }
        case CliCommand::UninstallService: {
            if (uninstall_service()) {
                MessageBoxA(nullptr, "Service uninstalled successfully.", "DupeCheck", MB_ICONINFORMATION);
            } else {
                MessageBoxA(nullptr, "Failed to uninstall service.", "DupeCheck", MB_ICONERROR);
            }
            break;
        }
        case CliCommand::RunService: {
            ServiceHost::run_service(L"C:\\", 300);
            break;
        }
        default: {
            // Run GUI.
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

            DatabaseManager db(db_path);
            if (!db.init()) {
                MessageBoxW(nullptr, L"Failed to initialize database.", L"Error", MB_ICONERROR);
                return 1;
            }

            // Initialize ImGui GUI.
            HINSTANCE hInst = GetModuleHandle(nullptr);
            run_gui(hInst, SW_SHOWDEFAULT, PathUtils::utf8_to_wide(L"C:\\"));

            HashEngine::cleanup();
            break;
        }
    }

    return 0;
}