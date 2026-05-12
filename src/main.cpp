#include <windows.h>
#include "database/DatabaseManager.h"
#include "gui/ImGuiView.h"
#include "hashing/HashEngine.h"
#include "service/ServiceHost.h"

static void initialize_database() {
    std::wstring db_path = get_default_db_path();
    DatabaseManager db(db_path);
    if (!db.init()) {
        MessageBoxW(nullptr, L"Failed to initialize database.", L"Error", MB_ICONERROR);
        exit(1);
    }
}

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
            initialize_database();
            HINSTANCE hInst = GetModuleHandle(nullptr);
            run_gui(hInst, SW_SHOWDEFAULT, L"C:\\");
            HashEngine::cleanup();
            return 0;
        }
    }

    return 0;
}
