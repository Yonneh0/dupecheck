#include "JsonConfig.h"
#include <fstream>
#include <sstream>
#include <algorithm>

std::string JsonConfig::trim_ws(const std::string& s) {
    auto start = std::find_if_not(s.begin(), s.end(), ::isspace);
    auto end = std::find_if_not(s.rbegin(), s.rend(), ::isspace).base();
    return (start < end ? std::string(start, end) : "");
}

std::unordered_map<std::string, std::string> JsonConfig::load(const std::wstring& path) {
    // Read directly as UTF-8 with ifstream to avoid wifstream conversion overhead.
    std::ifstream ifs(path);
    if (!ifs.is_open()) return {};

    std::unordered_map<std::string, std::string> config;
    std::string line;

    while (std::getline(ifs, line)) {
        line = trim_ws(line);
        if (line.empty() || line[0] == '#') continue;

        auto colon_pos = line.find(':');
        if (colon_pos == std::string::npos) continue;

        std::string key = trim_ws(line.substr(0, colon_pos));
        // Remove surrounding quotes from key.
        if (!key.empty() && key[0] == '"') {
            auto end_q = key.find('"', 1);
            if (end_q != std::string::npos) key = key.substr(1, end_q - 1);
        }

        // Remove trailing comma.
        std::string val = trim_ws(line.substr(colon_pos + 1));
        if (!val.empty() && val.back() == ',') {
            val.pop_back();
        }

        config[key] = val;
    }

    return config;
}

bool JsonConfig::save(const std::wstring& path, const std::unordered_map<std::string, std::string>& config) {
    // Write directly as UTF-8.
    std::ofstream ofs(path);
    if (!ofs.is_open()) return false;

    size_t idx = 0;
    for (const auto& [key, value] : config) {
        ofs << "  \"" << key << "\": \"" << value << "\"";
        // Check if last item.
        bool is_last = (++idx == config.size());
        ofs << (is_last ? "" : ",") << "\n";
    }

    return true;
}