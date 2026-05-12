// Minimal XXH32 implementation using the primes defined in xxhash.h.
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

XXH32_hash_t compute_xxhash32(const uint8_t* data, size_t len, uint32_t seed) {
    return XXH32(data, len, seed);
}

/* ===================== Streaming API Implementation ==================== */

XXH32_hash_t XXH32_reset(XXH32_state_t* state, uint32_t seed) {
    state->total_len = 0;
    state->seed      = seed;
    state->v1        = seed + XXH_PRIME32_1 + XXH_PRIME32_2;
    state->v2        = seed + XXH_PRIME32_2;
    state->v3        = seed + 0;
    state->v4        = seed - XXH_PRIME32_1;
    state->memsize   = 0;
    return XXH32_hash_t{0};
}

XXH32_hash_t XXH32_update(XXH32_state_t* state, const void* input, size_t length) {
    const uint8_t* p     = static_cast<const uint8_t*>(input);
    const uint8_t* bEnd  = p + length;

    state->total_len += length;

    if (state->memsize > 0) {
        /* Fill remaining buffer first. */
        while (state->memsize < 16 && p < bEnd) {
            state->mem[state->memsize++] = *p++;
        }
        if (state->memsize == 16) {
            /* Process the accumulated 16-byte block. */
            for (int i = 0; i < 4; ++i) {
                uint32_t key = *(uint32_t*)(state->mem + i * 4);
                key *= XXH_PRIME32_1; key = XXH_ROTL32(key, 7); key *= XXH_PRIME32_4;
                switch (i) {
                    case 0: state->v1 ^= key; break;
                    case 1: state->v2 ^= key; break;
                    case 2: state->v3 ^= key; break;
                    case 3: state->v4 ^= key; break;
                }
            }
            for (int i = 0; i < 4; ++i) {
                uint32_t* v = &state->v1 + i;
                *v += XXH_PRIME32_1 * (*(uint32_t*)(state->mem + i * 4));
                *v = XXH_ROTL32(*v, 13);
                *v *= XXH_PRIME32_3;
            }
            state->memsize = 0;
        }
    }

    if (p + 16 <= bEnd) {
        /* Process full 16-byte blocks. */
        const uint8_t* limit = bEnd - 15;
        while (p < limit) {
            for (int i = 0; i < 4; ++i) {
                uint32_t k = *(uint32_t*)(p + i * 4);
                switch (i) {
                    case 0: state->v1 += XXH_PRIME32_1 * k; break;
                    case 1: state->v2 += XXH_PRIME32_1 * k; break;
                    case 2: state->v3 += XXH_PRIME32_1 * k; break;
                    case 3: state->v4 += XXH_PRIME32_1 * k; break;
                }
            }
            p += 16;
            for (int i = 0; i < 4; ++i) {
                uint32_t* v = &state->v1 + i;
                *v = XXH_ROTL32(*v, 13);
                *v *= XXH_PRIME32_3;
            }
        }
    }

    /* Store remaining bytes. */
    while (p < bEnd) {
        state->mem[state->memsize++] = *p++;
    }

    return XXH32_hash_t{0};
}

XXH32_hash_t XXH32_digest(const XXH32_state_t* state) {
    const uint8_t* p     = state->mem;
    const uint8_t* bEnd  = p + state->memsize;
    uint32_t h32;

    if (state->total_len >= 16) {
        h32 = XXH_ROTL32(state->v1, 1) + XXH_ROTL32(state->v2, 7)
            + XXH_ROTL32(state->v3, 12) + XXH_ROTL32(state->v4, 18);
    } else {
        h32 = state->seed + XXH_PRIME32_5;
    }

    h32 += static_cast<uint32_t>(state->total_len);

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
