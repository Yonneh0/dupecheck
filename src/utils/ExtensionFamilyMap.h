#pragma once
#include <string>
#include <vector>
#include <unordered_map>

// Extension family mapping. Each extension maps to a "family" key, 
// and we can also look up all extensions in a family.
class ExtensionFamilyMap {
public:
    // Get the family for an extension. Returns reference that is valid until next call with unknown extension.
    static const std::string& get_family(const std::string& ext);
    
    // Get all extensions in the same family as `ext`.
    static std::vector<std::string> get_family_extensions(const std::string& ext);
    
    // Check if two extensions belong to the same family.
    static bool is_same_family(const std::string& a, const std::string& b) {
        return get_family(a) == get_family(b);
    }

private:
    static const std::vector<std::string>& get_family_extensions_cached(const std::string& ext);
    ExtensionFamilyMap() = delete; // Non-instantiable.
};

// Built-in mapping (used by default).
// Known families: image, document, spreadsheet, archive, video, audio.
inline const std::unordered_map<std::string, std::string>& builtin_families() {
    static const std::unordered_map<std::string, std::string> families = {
        // Images
        {"jpg", "image"}, {"jpeg", "image"}, {"jpe", "image"}, 
        {"png", "image"}, {"gif", "image"}, {"bmp", "image"},
        {"tiff", "image"}, {"tif", "image"}, {"webp", "image"},
        // Documents
        {"docx", "document"}, {"doc", "document"}, {"docm", "document"},
        {"xlsx", "spreadsheet"}, {"xls", "spreadsheet"}, {"csv", "spreadsheet"},
        {"pdf", "document"},
        // Archives
        {"zip", "archive"}, {"rar", "archive"}, {"7z", "archive"},
        {"tar", "archive"}, {"gz", "archive"},
        // Videos
        {"mp4", "video"}, {"avi", "video"}, {"mkv", "video"}, 
        {"mov", "video"}, {"wmv", "video"}, {"flv", "video"},
        // Audio
        {"mp3", "audio"}, {"wav", "audio"}, {"ogg", "audio"},
        {"m4a", "audio"}, {"aac", "audio"}, {"flac", "audio"},
    };
    return families;
}

// Get the family for an extension. For known extensions, returns a reference to
// the built-in map value (stable). For unknown extensions, returns a thread-local
// string that is valid until the next call with another unknown extension.
inline const std::string& ExtensionFamilyMap::get_family(const std::string& ext) {
    auto it = builtin_families().find(ext);
    if (it != builtin_families().end()) return it->second;

    // Thread-local fallback for unknown extensions — safe within a single thread,
    // but callers must copy the result before making nested calls.
    static thread_local std::string fallback;
    fallback = ext;
    return fallback;
}

inline std::vector<std::string> ExtensionFamilyMap::get_family_extensions(const std::string& ext) {
    auto it = builtin_families().find(ext);
    if (it == builtin_families().end()) return {ext}; // Unknown extension.

    const std::string& family = it->second;
    std::vector<std::string> result;
    for (const auto& [e, f] : builtin_families()) {
        if (f == family) result.push_back(e);
    }
    return result;
}

// Cached version of get_family_extensions to avoid rebuilding the vector every call.
inline const std::vector<std::string>& ExtensionFamilyMap::get_family_extensions_cached(const std::string& ext) {
    static thread_local std::unordered_map<std::string, std::vector<std::string>> cache;
    auto it = cache.find(ext);
    if (it != cache.end()) return it->second;

    auto result = get_family_extensions(ext);
    cache[ext] = std::move(result);
    return cache[ext];
}
