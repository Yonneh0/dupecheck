#pragma once
#include <cstdint>

// Detection strategies for the duplicate engine.
enum class Strategy : uint32_t {
    ExactMatch = 1,
    NameVariant = 2,
    SizeHashSimilar = 4,
    ExtensionFamily = 8,
    FolderCopy = 16,
};

constexpr uint32_t ALL_STRATEGIES = 0x1F;

inline constexpr int DEFAULT_NAME_SIMILARITY_THRESHOLD = 3;
inline constexpr uint32_t DEFAULT_HASH_TOLERANCE       = 1024;

struct StrategyConfig {
    int name_similarity_threshold = DEFAULT_NAME_SIMILARITY_THRESHOLD;
    uint32_t hash_tolerance       = DEFAULT_HASH_TOLERANCE;
};

// Convert a strategy flag to its display name.
inline const char* strategy_to_string(Strategy s) noexcept {
    switch (s) {
        case Strategy::ExactMatch:      return "Exact Match";
        case Strategy::NameVariant:     return "Name Variant";
        case Strategy::SizeHashSimilar: return "Size+Hash Similar";
        case Strategy::ExtensionFamily: return "Extension Family";
        case Strategy::FolderCopy:      return "Folder Copy";
    }
    return "Unknown";
}
