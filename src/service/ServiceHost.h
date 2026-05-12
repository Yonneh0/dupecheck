#pragma once
#include <string>
#include "../core/ActionModel.h"

/// Windows service for periodic duplicate scanning.
constexpr const wchar_t* SERVICE_NAME       = L"DupeCheck";
constexpr const wchar_t* DISPLAY_NAME       = L"DupeCheck Duplicate File Scanner";
constexpr int SCAN_INTERVAL_SECONDS         = 300;   // 5 minutes between scans

class ServiceHost {
public:
    /// Run the service with the given scan path.
    static void run_service(const std::wstring& scan_path, int interval_seconds);

private:
    inline static SERVICE_STATUS_HANDLE h_service_status_ = nullptr;
    inline static bool is_running_ = false;
    inline static std::wstring current_scan_path_;
};

/// Parse command-line arguments (returns CLI mode or GUI with optional scan path).
ServiceArgs parse_args(int argc, char** argv);
bool install_service(const wchar_t* exe_path, const wchar_t* scan_path);
bool uninstall_service();