// XXH32 implementation.
// The actual hash function body is inlined via #define XXH_IMPLEMENTATION in xxhash.h,
// so this file's purpose is to provide a single compilation unit for CMake linking.

#include "xxhash.h"

XXH32_hash_t compute_xxhash32(const uint8_t* data, size_t len, uint32_t seed) {
    return XXH32(data, len, seed);
}