#include <gtest/gtest.h>
#include "../src/engine/DuplicateEngine.h"
#include "../src/engine/ExactMatch.h"
#include "../src/engine/NameVariant.h"
#include "../src/engine/SizeHashSimilar.h"
#include "../src/engine/ExtensionFamily.h"

TEST(DuplicateEngineTest, ExactMatchBasic) {
    // Create a mock file list with some duplicates.
    std::vector<FileInfo> files = {
        {"file1.txt", 100, 1000},
        {"file2.txt", 100, 1000},
        {"file3.txt", 100, 1000},
    };

    // Set same SHA256 hash for all files (simulating exact match).
    auto sha = std::array<uint8_t, 32>{};
    std::fill(sha.begin(), sha.end(), 42);
    
    for (auto& f : files) {
        f.sha256 = sha;
        f.xxhash = XXH32(f.path.c_str(), wcslen(f.path), 0);
    }

    auto groups = exact_match(files);
    EXPECT_EQ(groups.size(), 1u);
    EXPECT_EQ(groups[0].files.size(), 3u);
}

TEST(DuplicateEngineTest, NameVariantDetection) {
    std::vector<FileInfo> files = {
        {"report_1.docx", 500, 2000},
        {"report_2.docx", 500, 2000},
    };

    auto sha = std::array<uint8_t, 32>{};
    std::fill(sha.begin(), sha.end(), 42);
    
    for (auto& f : files) {
        f.sha256 = sha;
        f.xxhash = XXH32(f.path.c_str(), wcslen(f.path), 0);
    }

    auto groups = name_variant(files, 3);
    // Should find at least one group.
    EXPECT_EQ(groups.size(), 1u);
}

TEST(DuplicateEngineTest, ExtensionFamilyDetection) {
    std::vector<FileInfo> files = {
        {"photo.jpg", 1000, 3000},
        {"photo.jpeg", 1000, 3000},
    };

    auto sha = std::array<uint8_t, 32>{};
    std::fill(sha.begin(), sha.end(), 42);
    
    for (auto& f : files) {
        f.sha256 = sha;
        f.xxhash = XXH32(f.path.c_str(), wcslen(f.path), 0);
    }

    auto groups = extension_family(files);
    // Should detect that jpg/jpeg are in the same family.
    EXPECT_EQ(groups.size(), 1u);
}

TEST(DuplicateEngineTest, NoDuplicates) {
    std::vector<FileInfo> files = {
        {"file1.txt", 100, 1000},
        {"file2.txt", 200, 2000},
        {"file3.txt", 300, 3000},
    };

    // Different SHA256 hashes.
    files[0].sha256 = {1};
    files[1].sha256 = {2};
    files[2].sha256 = {3};

    auto groups = exact_match(files);
    EXPECT_EQ(groups.size(), 0u);
}