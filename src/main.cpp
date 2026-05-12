#include <windows.h>
#include "database/DatabaseManager.h"
#include "gui/ImGuiView.h"
#include "hashing/HashEngine.h"
#include "service/ServiceHost.h"

// Entry point: parses CLI arguments and dispatches to either GUI or service mode.
int main() {
    HashEngine::init_bcrypt();
    ServiceArgs args = parse_args(__argc, __argv);

    switch (args.command) {
        case CliCommand::InstallService: {
            wchar_t exe_path[MAX_PATH];
            if (!GetModuleFileNameW(nullptr, exe_path, ARRAYSIZE(exe_path))) {
                MessageBoxA(nullptr, "Failed to determine executable path.", "DupeCheck", MB_ICONERROR);
                break;
            }
            install_service(exe_path, args.scan_path.c_str());
            MessageBoxA(nullptr, "Service installed successfully.", "DupeCheck", MB_ICONINFORMATION);
            break;
        }
        case CliCommand::UninstallService: {
            if (uninstall_service()) {
                MessageBoxA(nullptr, "Service uninstalled successfully.", "DupeCheck", MB_ICONINFORMATION);
            } else {
                MessageBoxA(nullptr, "Failed to uninstall service. Ensure the service is stopped first.", "DupeCheck", MB_ICONERROR);
            }
            break;
        }
        case CliCommand::RunService: {
            ServiceHost::run_service(args.scan_path.empty() ? L"C:\\" : args.scan_path, 300);
            break;
        }
        default:
            initialize_database();
            HashEngine::cleanup();
            return run_gui(GetModuleHandle(nullptr), SW_SHOWDEFAULT, args.scan_path.empty() ? L"" : args.scan_path.c_str());
    }

    HashEngine::cleanup();
    return 0;
}
