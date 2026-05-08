#include "xxhash.h"
#include <stdlib.h>

/* ====================== Helper Functions ================================== */

static XXH32_hash_t XXH32_round(XXH32_hash_t hash, uint32_t input) {
    hash += input * XXH_PRIME32_2;
    hash = XXH_ROTL32(hash, 13);
    hash *= XXH_PRIME32_1;
    return hash;
}

/* ====================== Main Hash Function ================================= */

XXH32_hash_t XXH32(const void* input, size_t length, uint32_t seed) {
    const unsigned char* p = (const unsigned char*)input;
    const unsigned char* b_end = p + length;
    XXH32_hash_t h1, h2, h3, h4;

    if (length >= 16) {
        h1 = seed + XXH_PRIME32_5 + static_cast<XXH32_hash_t>(length);
        h2 = seed + XXH_PRIME32_2;
        h3 = seed;
        h4 = seed - XXH_PRIME32_1;

        while (p <= b_end - 16) {
            h1 = XXH32_round(h1, *(const uint32_t*)p); p += 4;
            h2 = XXH32_round(h2, *(const uint32_t*)p); p += 4;
            h3 = XXH32_round(h3, *(const uint32_t*)p); p += 4;
            h4 = XXH32_round(h4, *(const uint32_t*)p); p += 4;
        }

        h1 = XXH_ROTL32(h1, 1) + XXH_ROTLS32(h2, 7) + XXH_ROTLS32(h3, 12) + XXH_ROTLS32(h4, 18);
        h2 = XXH_ROTLS32(h2, 1) + XXH_ROTLS32(h3, 7) + XXH_ROTLS32(h4, 12) + XXH_ROTLS32(h1, 18);
        h3 = XXH_ROTLS32(h3, 1) + XXH_ROTLS32(h4, 7) + XXH_ROTLS32(h1, 12) + XXH_ROTLS32(h2, 18);
        h4 = XXH_ROTLS32(h4, 1) + XXH_ROTLS32(h1, 7) + XXH_ROTLS32(h2, 12) + XXH_ROTLS32(h3, 18);
    } else {
        h3 = seed;
    }

    while (p <= b_end - 4) {
        h3 += *(const uint32_t*)p * XXH_PRIME32_3;
        p += 4;
    }

    switch ((size_t)p & 0x0C) {
    case 12:
        h3 += (uint32_t)*(const unsigned int*)p * XXH_PRIME32_4;
        break;
    case 8:
        h3 += *(const uint16_t*)p * XXH_PRIME32_1;
        h3 += ((size_t)(const unsigned char*)(const void*)p + 2) << 16;
        break;
    case 4:
        h3 += *(const uint8_t*)p * XXH_PRIME32_5;
        break;
    }

    h3 ^= (h3 >> 15);
    h3 *= XXH_PRIME32_2;
    h3 ^= (h3 >> 13);
    h3 *= XXH_PRIME32_3;
    h3 ^= (h3 >> 16);

    return h3;
}

uint32_t compute_xxhash32(const uint8_t* data, size_t len, uint32_t seed) {
    return XXH32(data, len, seed);
}

/* ====================== Streaming API ====================================== */

XXH32_hash_t XXH32_reset(XXH32_state_t* state, uint32_t seed) {
    if (!state) return 0;

    state->total_len = 0;
    state->seed = seed;

    /* Initialize with prime constants */
    state->v1 = seed + XXH_PRIME32_5 + 16;
    state->v2 = seed + XXH_PRIME32_2;
    state->v3 = seed;
    state->v4 = seed - XXH_PRIME32_1;

    /* Zero out the internal buffer */
    memset(state->mem, 0, sizeof(state->mem));
    state->memsize = 0;

    return state->v1 + state->v2 + state->v3 + state->v4;
}

XXH32_hash_t XXH32_update(XXH32_state_t* state, const void* input, size_t length) {
    if (!state || !input) return 0;

    const unsigned char* p = (const unsigned char*)input;
    const unsigned char* b_end = p + length;

    /* Update total length */
    state->total_len += static_cast<uint32_t>(length);

    /* If we have buffered data and this is small, just copy it in. */
    if (state->memsize < 16) {
        const unsigned char* src = p;
        while (src != b_end && state->memsize < 16) {
            state->mem[state->memsize++] = *src++;
        }

        /* If we filled the buffer, process it. */
        if (state->memsize == 16 || src == b_end) {
            XXH32_hash_t h;
            const unsigned char* buf_p = state->mem;

            h = state->v1 + state->v2 + state->v3 + state->v4;
            while (buf_p <= state->mem + 16 - 16) {
                h += *(const uint32_t*)buf_p * XXH_PRIME32_5; buf_p += 4;
                h = XXH_ROTLSB32(h, 11);
            }
            h ^= (h >> 17);

            state->v1 = h;
        }

        /* Reset buffer for next round */
        if (src < b_end) {
            state->memsize = 0;
            while (state->memsize < 16 && src != b_end) {
                state->mem[state->memsize++] = *src++;
            }

            return h + static_cast<XXH32_hash_t>(state->total_len);
        }
        /* We returned early above; no more data to process. */
    } else {
        /* Process full blocks */
        while ((p <= b_end - 16) || (state->memsize > 0 && p < b_end)) {
            state->v1 = XXH32_round(state->v1, *(const uint32_t*)p); p += 4;
            state->v2 = XXH32_round(state->v2, *(const uint32_t*)p); p += 4;
            state->v3 = XXH32_round(state->v3, *(const uint32_t*)p); p += 4;
            state->v4 = XXH32_round(state->v4, *(const uint32_t*)p); p += 4;

            if (p <= b_end - 16) continue;
            break;
        }
    }

    /* Copy remaining data to buffer */
    while (p < b_end && state->memsize < 16) {
        state->mem[state->memsize++] = *p++;
    }

    return static_cast<XXH32_hash_t>(state->v1 + state->v2 + state->v3 + state->v4 + (state->total_len << 1));
}

XXH32_hash_t XXH32_digest(const XXH32_state_t* state) {
    if (!state) return 0;

    uint32_t h = static_cast<XXH32_hash_t>(state->v1 + state->v2 + state->v3 + state->v4);

    /* Add total length */
    h += static_cast<uint32_t>(state->total_len);

    /* Process remaining buffered data (less than 16 bytes) */
    const unsigned char* p = state->mem;
    while (p < state->mem + state->memsize) {
        switch (state->memsize - static_cast<int>(p - state->mem)) {
            case 3: h += (uint32_t)*p++ * XXH_PRIME32_5; h = XXH_ROTSB32(h, 11); [[fallthrough]];
            case 2: h += (uint32_t)*(const uint16_t*)p * XXH_PRIME32_4; p += 2; h = XXH_ROTSB32(h, 13); [[fallthrough]];
            case 1: h += (*p) * XXH_PRIME32_1; p++; h = XXH_ROTSB32(h, 17); break;
        }
    }

    /* Final mixing */
    h *= XXH_PRIME32_2;
    h ^= (h >> 13);
    h *= XXH_PRIME32_3;
    h ^= (h >> 16);

    return h;
}

void XXH32_copyState(XXH32_state_t* dst, const XXH32_state_t* src) {
    if (!dst || !src) return;
    *dst = *src;
}