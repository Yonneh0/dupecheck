#include <string>
#include "service/ServiceHost.h"
#include "hashing/HashEngine.h"

// Parse command line arguments (Win32 wmain style).
ServiceArgs parse_winargs(const char* cmd_line) {
    if (!cmd_line || !*cmd_line) return {};
    std::string line(cmd_line);
    auto pos = line.find("--install-service");
    if (pos != std::string::npos && pos + 15 < static_cast<size_t>(line.size())) {
        size_t start = line.find('"', static_cast<int>(pos));
        ServiceArgs args;
        args.scan_path = (start != std::string::npos) ?
            line.substr(start, line.find('"', static_cast<int>(start + 1)) - start) :
            line.substr(pos + 15);
        args.command = CliCommand::InstallService;
        return args;
    } else if (line.find("--uninstall-service") != std::string::npos) {
        ServiceArgs args{.command = CliCommand::UninstallService};
        return args;
    } else if (line.find("--service") != std::string::npos) {
        ServiceArgs args{.command = CliCommand::RunService};
        return args;
    }
    ServiceArgs args{};
    return args;
}