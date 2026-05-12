#include "JsonConfig.h"
#include <fstream>
#include <sstream>

std::string JsonConfig::trim_ws(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return {};
    size_t end = s.find_last_not_of(" \t\n\r");
    return s.substr(start, end - start + 1);
}

std::unordered_map<std::string, std::string> JsonConfig::load(const std::wstring& path) {
    std::ifstream ifs(path);
    if (!ifs.is_open()) return {};

    std::unordered_map<std::string, std::string> config;
    std::string line;

    while (std::getline(ifs, line)) {
        line = trim_ws(line);
        if (line.empty() || line[0] == '#') continue;

        auto pos = line.find(':');
        if (pos == std::string::npos) continue;

        std::string key = trim_ws(line.substr(0, pos));
        // Strip quotes from key.
        if (!key.empty() && key.front() == '"') {
            size_t end_q = key.find('"', 1);
            if (end_q != std::string::npos) key = key.substr(1, end_q - 1);
        }

        std::string val = trim_ws(line.substr(pos + 1));
        // Strip trailing comma and quotes.
        while (!val.empty() && val.back() == ',') val.pop_back();
        if (val.size() >= 2 && val.front() == '"' && val.back() == '"') {
            val = val.substr(1, val.size() - 2);
        }

        config[key] = val;
    }
    return config;
}

bool JsonConfig::save(const std::wstring& path, const std::unordered_map<std::string, std::string>& config) {
    std::ofstream ofs(path);
    if (!ofs.is_open()) return false;

    size_t count = 0;
    for (const auto& [key, value] : config) {
        ofs << "  \"" << key << "\": " << value;
        bool is_last = (++count == config.size());
        ofs << (is_last ? "" : ",") << "\n";
    }
    return true;
}