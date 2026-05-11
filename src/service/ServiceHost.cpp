#include "ServiceHost.h"
#include <windows.h>
#include "../hashing/HashEngine.h"
#include "../scanner/FileScanner.h"
#include <algorithm>

SERVICE_STATUS_HANDLE ServiceHost::h_service_status_ = nullptr;
std::wstring ServiceHost::scan_path_;
bool ServiceHost::is_running_ = false;
int ServiceHost::g_interval_seconds_ = 300;

void ServiceHost::do_scan() {
    std::vector<PathUtils::FileEntry> entries;
    PathUtils::enumerate_files(scan_path_, entries);

    for (auto& entry : entries) {
        HashResult hr = HashEngine::compute(entry.path.c_str());
        (void)hr;
    }
}

DWORD WINAPI ServiceHost::service_main(DWORD argc, LPWSTR* argv) {
    h_service_status_ = RegisterServiceCtrlHandlerW(SERVICE_NAME, service_control_handler);

    SERVICE_STATUS ss;
    ss.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    ss.dwCurrentState = SERVICE_START_PENDING;
    ss.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    ss.dwWin32ExitCode = 0;
    ss.dwWaitHint = 5000;

    SetServiceStatus(h_service_status_, &ss);
    ss.dwCurrentState = SERVICE_RUNNING;

    HashEngine::init_bcrypt();

    is_running_ = true;
    do_scan();

    while (is_running_) {
        Sleep(static_cast<DWORD>(g_interval_seconds_) * 1000);
        if (is_running_) do_scan();
    }

    ss.dwCurrentState = SERVICE_STOPPED;
    SetServiceStatus(h_service_status_, &ss);
    return 0;
}

void WINAPI ServiceHost::service_control_handler(DWORD control) {
    switch (control) {
        case SERVICE_CONTROL_STOP:
            is_running_ = false;
            break;
        case SERVICE_CONTROL_SHUTDOWN:
            is_running_ = false;
            break;
    }

    SERVICE_STATUS ss;
    ss.dwCurrentState = (is_running_) ? SERVICE_RUNNING : SERVICE_STOPPED;
    SetServiceStatus(h_service_status_, &ss);
}

void ServiceHost::run_service(const std::wstring& scan_path, int interval_seconds) {
    h_service_status_ = RegisterServiceCtrlHandlerW(SERVICE_NAME, service_control_handler);
    if (h_service_status_ == nullptr) return;

    scan_path_ = scan_path;
    g_interval_seconds_ = interval_seconds;

    SERVICE_STATUS ss{};
    ss.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    ss.dwCurrentState = SERVICE_START_PENDING;
    ss.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    ss.dwWin32ExitCode = 0;
    ss.dwWaitHint = 5000;

    SetServiceStatus(h_service_status_, &ss);

    HashEngine::init_bcrypt();

    is_running_ = true;
    do_scan();

    ss.dwCurrentState = SERVICE_RUNNING;
    SetServiceStatus(h_service_status_, &ss);

    while (is_running_) {
        Sleep(static_cast<DWORD>(g_interval_seconds_) * 1000);
        if (is_running_) do_scan();
    }

    ss.dwCurrentState = SERVICE_STOPPED;
    SetServiceStatus(h_service_status_, &ss);
}

ServiceArgs parse_args(int argc, char** argv) {
    ServiceArgs args;
    if (argc < 1) return args;

    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);

        if (arg == "--install-service" && i + 1 < argc) {
            args.scan_path = argv[i + 1];
            args.command = CliCommand::InstallService;
        } else if (arg == "--uninstall-service") {
            args.command = CliCommand::UninstallService;
        } else if (arg == "--service") {
            args.command = CliCommand::RunService;
        }
    }

    return args;
}

bool install_service(const wchar_t* exe_path, const wchar_t* scan_path) {
    std::wstring full_exe_path;
    if (exe_path && *exe_path != L'\0') {
        full_exe_path = exe_path;
    } else {
        wchar_t szPath[MAX_PATH];
        DWORD len = GetModuleFileNameW(nullptr, szPath, ARRAYSIZE(szPath));
        full_exe_path = (len > 0) ? std::wstring(szPath) : L"dupecheck.exe";
    }

    SC_HANDLE sch = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CREATE_SERVICE);
    if (sch == nullptr) return false;

    std::wstring cmd_line = L"\"" + full_exe_path + L"\" --service";

    SC_HANDLE sh = CreateServiceW(sch, SERVICE_NAME, DISPLAY_NAME,
                                  SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS,
                                  SERVICE_AUTO_START, SERVICE_ERROR_NORMAL,
                                  cmd_line.c_str(), nullptr, nullptr, nullptr, nullptr, nullptr);

    bool ok = (sh != nullptr);
    CloseServiceHandle(sh);
    CloseServiceHandle(sch);

    return ok;
}

bool uninstall_service() {
    SC_HANDLE sch = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
    if (sch == nullptr) return false;

    SC_HANDLE sh = OpenServiceW(sch, SERVICE_NAME, SERVICE_STOP | DELETE);
    if (sh == nullptr) {
        CloseServiceHandle(sch);
        return false;
    }

    SERVICE_STATUS ss{};
    ControlService(sh, SERVICE_CONTROL_STOP, &ss);

    BOOL result = DeleteService(sh);
    CloseServiceHandle(sh);
    CloseServiceHandle(sch);

    return result != 0;
}