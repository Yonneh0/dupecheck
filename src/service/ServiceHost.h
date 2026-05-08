#pragma once
#include <string>
#include <vector>

// Service name for Windows service registration.
constexpr const wchar_t* SERVICE_NAME = L"DupeCheck";
constexpr const wchar_t* DISPLAY_NAME = L"DupeCheck Duplicate File Scanner";

enum class CliCommand {
    None,
    InstallService,
    UninstallService,
    RunService,
};

struct ServiceArgs {
    std::string scan_path;  // UTF-8 string from command line.
    bool installed = false;
    CliCommand command = CliCommand::None;
};

class ServiceHost {
public:
    static void run_service(const std::wstring& scan_path, int interval_seconds);

    // Windows service entry point (called when running as a registered service).
    static DWORD WINAPI service_main(DWORD argc, LPWSTR* argv);

    // Service control handler (callback for stop/shutdown events).
    static void WINAPI service_control_handler(DWORD control);

private:
    inline static SERVICE_STATUS_HANDLE h_service_status_ = nullptr;
    inline static std::wstring scan_path_;
    inline static bool is_running_ = false;
    inline static int g_interval_seconds_ = 300;

    static void do_scan();
};

// Parse command line arguments (console mode).
ServiceArgs parse_args(int argc, char** argv);

// Install the service to Windows registry.
bool install_service(const wchar_t* exe_path, const wchar_t* scan_path);

// Uninstall the service from Windows registry.
bool uninstall_service();