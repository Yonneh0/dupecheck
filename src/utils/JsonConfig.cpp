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
    // Open with wide-char mode for Windows paths.
    FILE* f = _wfopen(path.c_str(), L"r");
    if (!f) return {};

    std::unordered_map<std::string, std::string> config;
    char line[1024];

    while (fgets(line, sizeof(line), f)) {
        // Strip trailing newline.
        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) --len;
        line[len] = 0;

        std::string raw(line, len);
        raw = trim_ws(raw);
        if (raw.empty() || raw[0] == '#') continue;

        auto pos = raw.find(':');
        if (pos == std::string::npos) continue;

        std::string key = trim_ws(raw.substr(0, pos));
        // Strip quotes from key.
        if (!key.empty() && key.front() == '"') {
            size_t end_q = key.find('"', 1);
            if (end_q != std::string::npos) key = key.substr(1, end_q - 1);
        }

        std::string val = trim_ws(raw.substr(pos + 1));
        // Strip trailing comma.
        while (!val.empty() && val.back() == ',') val.pop_back();

        config[key] = val;
    }

    fclose(f);
    return config;
}

bool JsonConfig::save(const std::wstring& path, const std::unordered_map<std::string, std::string>& config) {
    FILE* f = _wfopen(path.c_str(), L"w");
    if (!f) return false;

    size_t count = 0;
    for (const auto& [key, value] : config) {
        fprintf(f, "  \"%s\": %s", key.c_str(), value.c_str());
        bool is_last = (++count == config.size());
        fputc(is_last ? '\n' : ',', f);
    }

    fclose(f);
    return true;
}