#pragma once

#include <string>
#include <unordered_map>
#include <vector>

class ExtensionFamilyMap {
public:
    static std::string get_family(const std::string& ext);
    static std::vector<std::string> get_family_extensions(const std::string& ext);
    static bool is_same_family(const std::string& a, const std::string& b) { return get_family(a) == get_family(b); }

private:
    ExtensionFamilyMap() = delete;

    static const std::unordered_map<std::string, std::string>& builtin_families();
};

inline const std::unordered_map<std::string, std::string>& ExtensionFamilyMap::builtin_families() {
    static const std::unordered_map<std::string, std::string> families = {
        {"jpg", "image"},     {"jpeg", "image"},  {"jpe", "image"},
        {"png", "image"},     {"gif", "image"},   {"bmp", "image"},
        {"tiff", "image"},    {"tif", "image"},   {"webp", "image"},
        {"docx", "document"}, {"doc", "document"},{"docm", "document"},
        {"xlsx", "spreadsheet"},{"xls","spreadsheet"}, {"csv","spreadsheet"},
        {"pdf","document"}, {"zip","archive"}, {"rar","archive"}, {"7z","archive"}, {"tar","archive"}, {"gz","archive"},
        {"mp4","video"},{"avi","video"},{"mkv","video"},{"mov","video"},{"wmv","video"},
        {"mp3","audio"},{"wav","audio"},{"ogg","audio"},{"m4a","audio"},{"aac","audio"},{"flac","audio"},
    };
    return families;
}

inline std::string ExtensionFamilyMap::get_family(const std::string& ext) {
    auto it = builtin_families().find(ext);
    if (it != builtin_families().end()) return it->second;
    return ext;
}

inline std::vector<std::string> ExtensionFamilyMap::get_family_extensions(const std::string& ext) {
    auto it = builtin_families().find(ext);
    if (it == builtin_families().end()) return {ext};
    const std::string& family = it->second;
    std::vector<std::string> result;
    for (const auto& [e, f] : builtin_families()) {
        if (f == family) result.push_back(e);
    }
    return result;
}
