#pragma once
#include <cstdint>

enum class Strategy : uint32_t {
    ExactMatch = 1,
    NameVariant = 2,
    SizeHashSimilar = 4,
    ExtensionFamily = 8,
    FolderCopy = 16,
};

constexpr uint32_t ALL_STRATEGIES = 0x1F;

struct StrategyConfig {
    int name_similarity_threshold = 3;
    uint32_t hash_tolerance = 1024;
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