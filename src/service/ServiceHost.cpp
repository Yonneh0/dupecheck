#include "ServiceHost.h"
#include <windows.h>
#include "../hashing/HashEngine.h"
#include "../scanner/FileScanner.h"

SERVICE_STATUS_HANDLE ServiceHost::h_service_status_ = nullptr;
std::wstring ServiceHost::scan_path_;
bool ServiceHost::is_running_ = false;
int ServiceHost::g_interval_seconds_ = 300;

static void service_do_scan() {
    std::vector<PathUtils::FileEntry> entries; PathUtils::enumerate_files(ServiceHost::scan_path_, entries);
    for (auto& entry : entries) HashEngine::compute(entry.path.c_str());
}

static SERVICE_STATUS get_status(bool running) {
    SERVICE_STATUS ss{}; ss.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    ss.dwCurrentState = running ? SERVICE_RUNNING : SERVICE_STOPPED;
    ss.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    return ss;
}

DWORD WINAPI ServiceHost::service_main(DWORD /*argc*/, LPWSTR* /*argv*/) {
    h_service_status_ = RegisterServiceCtrlHandlerW(SERVICE_NAME, service_control_handler);
    if (!h_service_status_) return 1;
    SERVICE_STATUS ss = get_status(false); ss.dwCurrentState = SERVICE_START_PENDING; ss.dwWaitHint = 5000; SetServiceStatus(h_service_status_, &ss);
    HashEngine::init_bcrypt(); is_running_ = true; service_do_scan();
    ss.dwCurrentState = SERVICE_RUNNING; SetServiceStatus(h_service_status_, &ss);
    while (is_running_) { Sleep(static_cast<DWORD>(g_interval_seconds_) * 1000); if (is_running_) service_do_scan(); }
    ss.dwCurrentState = SERVICE_STOPPED; SetServiceStatus(h_service_status_, &ss); return 0;
}

void WINAPI ServiceHost::service_control_handler(DWORD control) {
    switch (control) { case SERVICE_CONTROL_STOP: case SERVICE_CONTROL_SHUTDOWN: is_running_ = false; break; }
    SetServiceStatus(h_service_status_, &get_status(is_running_));
}

void ServiceHost::run_service(const std::wstring& scan_path, int interval_seconds) {
    h_service_status_ = RegisterServiceCtrlHandlerW(SERVICE_NAME, service_control_handler);
    if (!h_service_status_) return; scan_path_ = scan_path; g_interval_seconds_ = interval_seconds;
    SERVICE_STATUS ss{}; ss.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    ss.dwCurrentState = SERVICE_START_PENDING; ss.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    ss.dwWaitHint = 5000; SetServiceStatus(h_service_status_, &ss); HashEngine::init_bcrypt(); is_running_ = true;
    service_do_scan(); ss.dwCurrentState = SERVICE_RUNNING; SetServiceStatus(h_service_status_, &ss);
    while (is_running_) { Sleep(static_cast<DWORD>(g_interval_seconds_) * 1000); if (is_running_) service_do_scan(); }
    ss.dwCurrentState = SERVICE_STOPPED; SetServiceStatus(h_service_status_, &ss);
}

ServiceArgs parse_args(int argc, char** argv) {
    ServiceArgs args; if (argc < 1) return args;
    for (int i = 1; i < argc; ++i) { std::string arg(argv[i]);
        if (arg == "--install-service" && i + 1 < static_cast<size_t>(argc)) { args.scan_path = argv[i+1]; args.command = CliCommand::InstallService; }
        else if (arg == "--uninstall-service") args.command = CliCommand::UninstallService;
        else if (arg == "--service") args.command = CliCommand::RunService;
    } return args;
}

bool install_service(const wchar_t* exe_path, const wchar_t*) {
    std::wstring full_exe_path = (exe_path && *exe_path) ? exe_path : []{ wchar_t sz[MAX_PATH]; if (GetModuleFileNameW(nullptr,sz,ARRAYSIZE(sz))) return std::wstring(sz); return L"dupecheck.exe"; }();
    SC_HANDLE sch = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CREATE_SERVICE); if (!sch) return false;
    SC_HANDLE sh = CreateServiceW(sch, SERVICE_NAME, DISPLAY_NAME, SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START, SERVICE_ERROR_NORMAL, full_exe_path.c_str(), nullptr, nullptr, nullptr, nullptr, nullptr);
    bool ok = (sh != nullptr); CloseServiceHandle(sh); CloseServiceHandle(sch); return ok;
}

bool uninstall_service() {
    SC_HANDLE sch = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_ALL_ACCESS); if (!sch) return false;
    SC_HANDLE sh = OpenServiceW(sch, SERVICE_NAME, SERVICE_STOP | DELETE); if (!sh) { CloseServiceHandle(sch); return false; }
    ControlService(sh, SERVICE_CONTROL_STOP, nullptr); BOOL result = DeleteService(sh); CloseServiceHandle(sh); CloseServiceHandle(sch); return result != 0;
}