#include <string>
#include "service/ServiceHost.h"

// Parse command line arguments.
inline ServiceArgs parse_winargs(const char* cmd_line) {
    ServiceArgs args; if (!cmd_line || !*cmd_line) return args;
    std::string line(cmd_line); auto pos = line.find("--install-service");
    if (pos != std::string::npos && pos + 15 < static_cast<size_t>(line.size())) {
        size_t start = line.find('"', static_cast<int>(pos));
        args.scan_path = (start != std::string::npos) ? line.substr(start, line.find('"', static_cast<int>(start+1)) - start) : line.substr(pos + 15);
        args.command = CliCommand::InstallService;
    } else if (line.find("--uninstall-service") != std::string::npos) { args.command = CliCommand::UninstallService; }
    else if (line.find("--service") != std::string::npos) { args.command = CliCommand::RunService; }
    return args;
}

// Overload that accepts argc.
inline ServiceArgs parse_winargs(const char* cmd_line, int /*argc*/) { return parse_winargs(cmd_line); }