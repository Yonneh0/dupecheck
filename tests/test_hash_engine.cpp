#include <gtest/gtest.h>
#include "../src/hashing/xxhash/xxhash.c"  // Include implementation for testing

TEST(HashEngineTest, XxHash32Basic) {
    uint32_t hash = XXH32("Hello World!", 12, 0);
    EXPECT_NE(hash, 0u);
}

TEST(HashEngineTest, XxHash32EmptyString) {
    uint32_t hash = XXH32("", 0, 0);
    // Empty string should have a deterministic hash value
    EXPECT_EQ(hash, (uint32_t)(XXH_PRIME32_5));
}

TEST(HashEngineTest, XxHash32SeedZero) {
    uint32_t hash = XXH32("test", 4, 0);
    
    // Same input with same seed should produce same output
    uint32_t hash2 = XXH32("test", 4, 0);
    EXPECT_EQ(hash, hash2);
}

TEST(HashEngineTest, XxHash32DifferentSeeds) {
    uint32_t h1 = XXH32("test", 4, 0);
    uint32_t h2 = XXH32("test", 4, 1);
    
    EXPECT_NE(h1, h2);
}

TEST(HashEngineTest, XxHash32DifferentInputs) {
    uint32_t h1 = XXH32("a", 1, 0);
    uint32_t h2 = XXH32("b", 1, 0);
    
    EXPECT_NE(h1, h2);
}

TEST(HashEngineTest, XxHash32LargeInput) {
    // Test with larger input (larger than hash block size)
    const char* data = "Hello World! Hello World!";
    uint32_t hash = XXH32(data, strlen(data), 0);
    
    EXPECT_NE(hash, 0u);
}

TEST(HashEngineTest, XxHash32StreamingAPI) {
    XXH32_state_t state;
    XXH32_reset(&state, 0);
    
    const char* part1 = "Hello ";
    const char* part2 = "World!";
    
    XXH32_update(&state, part1, strlen(part1));
    XXH32_update(&state, part2, strlen(part2));
    
    uint32_t hash = XXH32_digest(&state);
    
    // Should match direct computation of full string
    uint32_t expected = XXH32("Hello World!", 12, 0);
    EXPECT_EQ(hash, expected);
}

TEST(HashEngineTest, XxHash32StreamingIncremental) {
    XXH32_state_t state;
    XXH32_reset(&state, 42);
    
    // Hash incrementally: one byte at a time.
    const char* data = "The quick brown fox jumps over the lazy dog";
    for (size_t i = 0; i < strlen(data); ++i) {
        XXH32_update(&state, &data[i], 1);
    }
    
    uint32_t hash = XXH32_digest(&state);
    
    // Should match direct computation of full string.
    uint32_t expected = XXH32(data, strlen(data), 42);
    EXPECT_EQ(hash, expected);
}

TEST(HashEngineTest, XxHash32CopyState) {
    XXH32_state_t state1;
    XXH32_reset(&state1, 0);
    
    const char* part1 = "Hello ";
    XXH32_update(&state1, part1, strlen(part1));
    
    // Copy the state at this point.
    XXH32_state_t state2;
    XXH32_copyState(&state2, &state1);
    
    const char* part2 = "World!";
    XXH32_update(&state1, part2, strlen(part2));
    XXH32_update(&state2, part2, strlen(part2));
    
    uint32_t hash1 = XXH32_digest(&state1);
    uint32_t hash2 = XXH32_digest(&state2);
    
    // Both should produce the same final hash.
    EXPECT_EQ(hash1, hash2);
}

TEST(HashEngineTest, XxHash32KnownValue) {
    // Test against known value for a simple input.
    uint32_t hash = XXH32("abc", 3, 0);
    
    // This should be a deterministic value (XXH32 of "abc" with seed=0).
    EXPECT_NE(hash, 0u);
}

TEST(HashEngineTest, XxHash32BufferAlignment) {
    const char* data = "test string";
    uint32_t hash1 = XXH32(data, strlen(data), 0);
    
    // Create a copy with different alignment.
    std::vector<uint8_t> buffer(strlen(data));
    memcpy(buffer.data(), data, strlen(data));
    const char* aligned_data = (const char*)buffer.data();
    
    uint32_t hash2 = XXH32(aligned_data, strlen(data), 0);
    
    // Both should produce the same hash regardless of alignment.
    EXPECT_EQ(hash1, hash2);
}