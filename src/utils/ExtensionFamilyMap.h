#pragma once
#include <string>
#include <unordered_map>

// Extension family mapping for detecting duplicates across related extensions (e.g., jpg/jpeg).
class ExtensionFamilyMap {
public:
    static std::string get_family(const std::string& ext) {
        auto it = builtin_families().find(ext);
        return (it != builtin_families().end()) ? it->second : ext;
    }

    static bool is_same_family(const std::string& a, const std::string& b) {
        return get_family(a) == get_family(b);
    }

private:
    ExtensionFamilyMap() = delete;
    static const std::unordered_map<std::string, std::string>& builtin_families();
};

inline const std::unordered_map<std::string, std::string>& ExtensionFamilyMap::builtin_families() {
    // Group extensions by their common category.
    static const std::unordered_map<std::string, std::string> families = {
        {"jpg", "image"},     {"jpeg", "image"},  {"jpe", "image"},
        {"png", "image"},     {"gif", "image"},   {"bmp", "image"},
        {"tiff", "image"},    {"tif", "image"},   {"webp", "image"},
        {"docx", "document"}, {"doc", "document"},{"docm", "document"},
        {"xlsx", "spreadsheet"},{"xls","spreadsheet"}, {"csv","spreadsheet"},
        {"pdf","document"},
        {"zip","archive"}, {"rar","archive"}, {"7z","archive"}, {"tar","archive"}, {"gz","archive"},
        {"mp4","video"},{"avi","video"},{"mkv","video"},{"mov","video"},{"wmv","video"},
        {"mp3","audio"},{"wav","audio"},{"ogg","audio"},{"m4a","audio"},{"aac","audio"},{"flac","audio"},
    };
    return families;
}