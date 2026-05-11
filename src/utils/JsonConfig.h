#pragma once
#include <string>
#include <unordered_map>
#include <vector>

class JsonConfig {
public:
    static std::unordered_map<std::string, std::string> load(const std::wstring& path);
    static bool save(const std::wstring& path, const std::unordered_map<std::string, std::string>& config);
private:
    static std::string trim_ws(const std::string& s);
};