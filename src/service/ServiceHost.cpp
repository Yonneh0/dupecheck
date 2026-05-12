#include "ServiceHost.h"
#include <windows.h>
#include "../hashing/HashEngine.h"

static void service_do_scan(const std::wstring& scan_path) {
    static std::vector<PathUtils::FileInfo> entries;
    entries.clear();
    PathUtils::enumerate_files(scan_path, entries);
}

void ServiceHost::run_service(const std::wstring& scan_path, int interval_seconds) {
    h_service_status_ = RegisterServiceCtrlHandlerW(SERVICE_NAME, [](DWORD control) {
        switch (control) {
            case SERVICE_CONTROL_STOP:
            case SERVICE_CONTROL_SHUTDOWN:
                is_running_ = false;
                break;
        }
        SetServiceStatus(h_service_status_, &get_status(is_running_));
    });

    if (!h_service_status_) return;

    SERVICE_STATUS ss{};
    ss.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    ss.dwCurrentState = SERVICE_START_PENDING;
    ss.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    ss.dwWaitHint = 5000;
    SetServiceStatus(h_service_status_, &ss);

    HashEngine::init_bcrypt();
    is_running_ = true;
    service_do_scan(scan_path);

    ss.dwCurrentState = SERVICE_RUNNING;
    SetServiceStatus(h_service_status_, &ss);

    while (is_running_) {
        Sleep(static_cast<DWORD>(interval_seconds) * 1000);
        if (is_running_) service_do_scan(scan_path);
    }

    ss.dwCurrentState = SERVICE_STOPPED;
    SetServiceStatus(h_service_status_, &ss);
}

static SERVICE_STATUS get_status(bool running) {
    SERVICE_STATUS ss{};
    ss.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    ss.dwCurrentState = running ? SERVICE_RUNNING : SERVICE_STOPPED;
    ss.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    return ss;
}

ServiceArgs parse_args(int argc, char** argv) {
    ServiceArgs args;
    if (argc < 1) return args;
    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "--install-service" && i + 1 < static_cast<size_t>(argc)) {
            args.scan_path = PathUtils::utf8_to_wide(argv[i+1]);
            args.command = CliCommand::InstallService;
        } else if (arg == "--uninstall-service") {
            args.command = CliCommand::UninstallService;
        } else if (arg == "--service") {
            args.command = CliCommand::RunService;
        }
    }
    return args;
}

bool install_service(const wchar_t* exe_path, const wchar_t*) {
    std::wstring full_exe_path = (exe_path && *exe_path) ? exe_path : []() {
        wchar_t sz[MAX_PATH];
        if (GetModuleFileNameW(nullptr, sz, ARRAYSIZE(sz))) return std::wstring(sz);
        return L"dupecheck.exe";
    }();

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