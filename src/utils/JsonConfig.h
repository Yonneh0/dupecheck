#pragma once
#include <string>
#include <unordered_map>
#include <vector>

// Lightweight JSON config reader (no external library needed).
class JsonConfig {
public:
    // Load settings from file.
    static std::unordered_map<std::string, std::string> load(const std::wstring& path);
    
    // Save settings to file as formatted JSON.
    static bool save(const std::wstring& path, const std::unordered_map<std::string, std::string>& config);

private:
    static std::string trim_ws(const std::string& s);
};