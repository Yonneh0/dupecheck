#pragma once
#include <cstdint>
#include <string>

/// Detection strategies — each uses a different approach to identify duplicates.
enum class Strategy : uint32_t {
    ExactMatch = 1,         // SHA256 match (exact byte-for-byte copy).
    NameVariant = 2,        // Same content + name within Levenshtein distance threshold.
    SizeHashSimilar = 4,    // Similar size + XxHash32 in the same bin range.
    ExtensionFamily = 8,    // Same content across related extensions (e.g., jpg/jpeg).
    FolderCopy = 16,        // Entire directory trees copied to new locations.
};

/// Configuration for duplicate detection strategies.
struct StrategyConfig {
    int name_similarity_threshold = 3;  /// Max Levenshtein distance for name-variant matching.
    uint32_t hash_tolerance = 1024;     /// XxHash32 bin tolerance in bytes (Size+Hash strategy).
    bool service_enabled = true;         /// Whether the Windows service is active.
};

// Convert Strategy enum to human-readable string.
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