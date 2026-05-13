#include "xxhash.h"

XXH32_hash_t XXH32(const void* input, size_t length, uint32_t seed) {
    const uint8_t* p = (const uint8_t*)input;
    const uint8_t* bEnd = p + length;
    uint32_t h32;

    if (length >= 16) {
        const uint8_t* limit = bEnd - 12;
        uint32_t v1 = seed + XXH_PRIME32_1 + XXH_PRIME32_2;
        uint32_t v2 = seed + XXH_PRIME32_2;
        uint32_t v3 = seed + 0;
        uint32_t v4 = seed - XXH_PRIME32_1;

        do {
            v1 += XXH_PRIME32_1 * (*(uint32_t*)(p)); p += 4;
            v1 = XXH_ROTL32(v1, 13);
            v1 *= XXH_PRIME32_3;

            v2 += XXH_PRIME32_1 * (*(uint32_t*)(p)); p += 4;
            v2 = XXH_ROTL32(v2, 13);
            v2 *= XXH_PRIME32_3;

            v3 += XXH_PRIME32_1 * (*(uint32_t*)(p)); p += 4;
            v3 = XXH_ROTL32(v3, 13);
            v3 *= XXH_PRIME32_3;

            v4 += XXH_PRIME32_1 * (*(uint32_t*)(p)); p += 4;
            v4 = XXH_ROTL32(v4, 13);
            v4 *= XXH_PRIME32_3;
        } while (p <= limit);

        h32 = XXH_ROTL32(v1, 1) + XXH_ROTL32(v2, 7) + XXH_ROTL32(v3, 12) + XXH_ROTL32(v4, 18);
    } else {
        h32 = seed + XXH_PRIME32_5;
    }

    h32 += (uint32_t)length;

    while (p + 4 <= bEnd) {
        uint32_t k1 = *(uint32_t*)(p); p += 4;
        k1 *= XXH_PRIME32_1;
        k1 = XXH_ROTL32(k1, 7);
        k1 *= XXH_PRIME32_4;
        h32 ^= k1;
        h32 = XXH_ROTL32(h32, 3) * XXH_PRIME32_2 + XXH_PRIME32_3;
    }

    while (p < bEnd) {
        h32 ^= (*p) * XXH_PRIME32_5;
        h32 = XXH_ROTL32(h32, 11) * XXH_PRIME32_1;
        p++;
    }

    h32 ^= h32 >> 15;
    h32 *= XXH_PRIME32_2;
    h32 ^= h32 >> 13;
    h32 *= XXH_PRIME32_3;
    h32 ^= h32 >> 16;

    return h32;
}

uint32_t compute_xxhash32(const uint8_t* data, size_t len, uint32_t seed) {
    return XXH32(data, len, seed);
}