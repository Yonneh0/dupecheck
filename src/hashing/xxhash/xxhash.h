#pragma once
#include <stdint.h>
#include <string.h>

/* ===================== XXH32 — Non-Cryptographic Hash ==================== */

#define XXH_PRIME32_1 0x9E3779B1u
#define XXH_PRIME32_2 0x85EBCA77u
#define XXH_PRIME32_3 0xC2B2AE3Du
#define XXH_PRIME32_4 0x27D4EB2Fu
#define XXH_PRIME32_5 0x165667B1u

#define XXH_ROTL32(x, n)   ((x << (n)) | (x >> (32 - (n))))

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t XXH32_hash_t;

XXH32_hash_t XXH32(const void* input, size_t length, uint32_t seed);

/* Convenience function — computes the hash of raw bytes. */
uint32_t compute_xxhash32(const uint8_t* data, size_t len, uint32_t seed);

/* ===================== Streaming API ==================================== */

typedef struct {
    unsigned long long total_len;
    uint32_t seed;
    uint32_t v1, v2, v3, v4;
    unsigned char mem[16];
    int memsize;
} XXH32_state_t;

XXH32_hash_t XXH32_reset(XXH32_state_t* state, uint32_t seed);
XXH32_hash_t XXH32_update(XXH32_state_t* state, const void* input, size_t length);
XXH32_hash_t XXH32_digest(const XXH32_state_t* state);

#ifdef __cplusplus
}
#endif
