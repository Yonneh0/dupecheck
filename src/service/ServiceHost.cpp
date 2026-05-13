#include "ServiceHost.h"
#include <windows.h>
#include "../core/FileInfo.h"
#include "../hashing/HashEngine.h"
#include "../scanner/CachedScannerService.h"
#include "../engine/DuplicateEngine.h"
#include "../database/DatabaseManager.h"
#include "../gui/SettingsDialog.h"

static SERVICE_STATUS make_status(DWORD state, DWORD controls = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN) {
    SERVICE_STATUS ss{};
    ss.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    ss.dwCurrentState = state;
    ss.dwControlsAccepted = controls;
    return ss;
}

void ServiceHost::run_service(const std::wstring& scan_path, int interval_seconds) {
    current_scan_path_ = scan_path;

    h_service_status_ = RegisterServiceCtrlHandlerW(SERVICE_NAME, [](DWORD control) {
        if (control == SERVICE_CONTROL_STOP || control == SERVICE_CONTROL_SHUTDOWN) {
            is_running_ = false;
        }
        SERVICE_STATUS ss = make_status(is_running_ ? SERVICE_RUNNING : SERVICE_STOPPED);
        SetServiceStatus(h_service_status_, &ss);
    });

    if (!h_service_status_) return;

    { SERVICE_STATUS ss = make_status(SERVICE_START_PENDING, 0); SetServiceStatus(h_service_status_, &ss); }

    HashEngine::init_bcrypt();

    CachedScannerService scanner;
    DatabaseManager db{get_default_db_path()};
    if (!db.init()) {
        is_running_ = false;
        SERVICE_STATUS ss = make_status(SERVICE_STOPPED);
        SetServiceStatus(h_service_status_, &ss);
        return;
    }

    is_running_ = true;
    { SERVICE_STATUS ss = make_status(SERVICE_RUNNING); SetServiceStatus(h_service_status_, &ss); }

    while (is_running_) {
        Sleep(static_cast<DWORD>(interval_seconds) * 1000);
        if (!is_running_ || current_scan_path_.empty()) continue;

        if (scanner.init()) {
            auto files = scanner.scan(current_scan_path_.c_str());
            DuplicateEngine engine{get_strategy_config()};
            auto groups = engine.find_duplicates(files, ALL_STRATEGIES);

            int64_t path_hash = 0;
            for (wchar_t c : current_scan_path_) path_hash += static_cast<int64_t>(c);
            std::string scan_path_utf8 = PathUtils::wide_to_utf8(current_scan_path_);
            db.save_session(path_hash, scan_path_utf8, static_cast<int>(files.size()),
                           static_cast<int>(groups.size()), ALL_STRATEGIES);
        }
    }

    { SERVICE_STATUS ss = make_status(SERVICE_STOPPED); SetServiceStatus(h_service_status_, &ss); }
}

ServiceArgs parse_args(int argc, char** argv) {
    ServiceArgs args;
    if (argc < 1) return args;
    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "--install-service" && i + 1 < argc) {
            args.scan_path = PathUtils::utf8_to_wide(argv[i+1]);
            args.command = CliCommand::InstallService;
            ++i; // skip the path argument
        } else if (arg == "--uninstall-service") {
            args.command = CliCommand::UninstallService;
        } else if (arg == "--service") {
            args.command = CliCommand::RunService;
        } else if (arg[0] != '-') {
            // Treat bare arguments as scan paths
            args.scan_path = PathUtils::utf8_to_wide(argv[i]);
        }
    }
    return args;
}

bool install_service(const wchar_t* exe_path) {
    std::wstring full_exe_path;
    if (exe_path && *exe_path) {
        full_exe_path = exe_path;
    } else {
        wchar_t sz[MAX_PATH];
        if (GetModuleFileNameW(nullptr, sz, ARRAYSIZE(sz))) {
            full_exe_path = sz;
        } else {
            full_exe_path = L"dupecheck.exe";
        }
    }

    SC_HANDLE sch = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CREATE_SERVICE);
    if (!sch) return false;

    SC_HANDLE sh = CreateServiceW(sch, SERVICE_NAME, DISPLAY_NAME,
                                   SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS,
                                   SERVICE_AUTO_START, SERVICE_ERROR_NORMAL,
                                   full_exe_path.c_str(), nullptr, nullptr, nullptr, nullptr, nullptr);
    bool ok = (sh != nullptr);
    CloseServiceHandle(sh);
    CloseServiceHandle(sch);
    return ok;
}

bool uninstall_service() {
    SC_HANDLE sch = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
    if (!sch) return false;

    SC_HANDLE sh = OpenServiceW(sch, SERVICE_NAME, SERVICE_STOP | DELETE);
    if (!sh) { CloseServiceHandle(sch); return false; }

    ControlService(sh, SERVICE_CONTROL_STOP, nullptr);
    BOOL result = DeleteService(sh);
    CloseServiceHandle(sh);
    CloseServiceHandle(sch);
    return result != 0;
}