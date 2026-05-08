#include <string>
#include <cstring>
#include "service/ServiceHost.h"  // For ServiceArgs and CliCommand types.

// CLI argument parsing for GUI mode (uses lpCmdLine).
inline ServiceArgs parse_winargs(const char* cmd_line) {
    ServiceArgs args;
    if (!cmd_line || !*cmd_line) return args;

    std::string line(cmd_line);

    auto pos = line.find("--install-service");
    if (pos != std::string::npos && pos + 15 < static_cast<size_t>(line.size())) {
        size_t start = line.find('"', static_cast<int>(pos));
        if (start == std::string::npos) start = pos;
        else {
            auto end_q = line.find('"', static_cast<int>(start + 1));
            args.scan_path = (end_q != std::string::npos) ?
                             line.substr(start, end_q - start) : line.substr(start + 1);
            args.command = CliCommand::InstallService;
        }
    } else if ((pos = line.find("--uninstall-service")) != std::string::npos) {
        args.command = CliCommand::UninstallService;
    } else if ((pos = line.find("--service")) != std::string::npos) {
        args.command = CliCommand::RunService;
    }

    return args;
}
