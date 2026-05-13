#pragma once
#include <string>
#include <unordered_map>

/// Lightweight JSON config reader/writer for settings.json.
class JsonConfig {
public:
    /// Load key-value pairs from a JSON file. Values are returned as raw strings (no type parsing).
    static std::unordered_map<std::string, std::string> load(const std::wstring& path);

    /// Save the given config map to disk. Integers are written without quotes; string values are quoted.
    static bool save(const std::wstring& path, const std::unordered_map<std::string, std::string>& config);

private:
    static std::string trim_ws(const std::string& s);
    static std::string parse_value(const std::string& raw);
};
