#pragma once
#include "../hashing/xxhash/xxhash.h"
#include <cstdint>
#include <cstring>

// Convenience wrapper around XXH32 for use from C++ code.
inline uint32_t xxh32(const void* data, size_t len, uint32_t seed) {
    return compute_xxhash32(static_cast<const uint8_t*>(data), len, seed);
}
