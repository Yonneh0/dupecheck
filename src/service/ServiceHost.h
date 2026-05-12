#pragma once
#include <string>

// Windows Service for periodic duplicate scanning.
constexpr const wchar_t* SERVICE_NAME = L"DupeCheck";
constexpr const wchar_t* DISPLAY_NAME = L"DupeCheck Duplicate File Scanner";

enum class CliCommand { None, InstallService, UninstallService, RunService };

struct ServiceArgs { std::string scan_path; bool installed = false; CliCommand command = CliCommand::None };

class ServiceHost {
public:
    static void run_service(const std::wstring& scan_path, int interval_seconds);
private:
    inline static SERVICE_STATUS_HANDLE h_service_status_ = nullptr;
    inline static std::wstring scan_path_;
    inline static bool is_running_ = false;
    inline static int g_interval_seconds_ = 300;
};

// Parse command-line arguments.
ServiceArgs parse_args(int argc, char** argv);
bool install_service(const wchar_t* exe_path, const wchar_t* scan_path);
bool uninstall_service();