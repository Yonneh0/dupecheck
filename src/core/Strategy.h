#pragma once
#include <cstdint>
#include <string>

enum class Strategy : uint32_t {
    ExactMatch = 1,
    NameVariant = 2,
    SizeHashSimilar = 4,
    ExtensionFamily = 8,
    FolderCopy = 16,
};

struct StrategyConfig {
    int name_similarity_threshold = 3;   // Levenshtein distance for name-variant detection.
    uint32_t hash_tolerance = 1024;      // XxHash32 bin size (bytes) for similarity grouping.
    bool service_enabled = true;         // Whether the Windows service should run.
};

inline const char* strategy_to_string(Strategy s) {
    switch (s) {
        case Strategy::ExactMatch:      return "Exact Match";
        case Strategy::NameVariant:     return "Name Variant";
        case Strategy::SizeHashSimilar: return "Size+Hash Similar";
        case Strategy::ExtensionFamily: return "Extension Family";
        case Strategy::FolderCopy:      return "Folder Copy";
    }
    return "Unknown";
}