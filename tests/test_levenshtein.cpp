#include <gtest/gtest.h>
#include "../src/utils/Levenshtein.h"

TEST(LevenshteinTest, EmptyStrings) {
    EXPECT_EQ(levenshtein_distance(L"", L""), 0);
}

TEST(LevenshteinTest, IdentityDistance) {
    EXPECT_EQ(levenshtein_distance(L"hello", L"hello"), 0);
}

TEST(LevenshteinTest, SingleCharInsert) {
    EXPECT_EQ(levenshtein_distance(L"", L"a"), 1);
    EXPECT_EQ(levenshtein_distance(L"hello", L"hallo"), 1);
}

TEST(LevenshteinTest, FullEditDistance) {
    // "abc" -> "def" = max distance (3 chars different)
    EXPECT_EQ(levenshtein_distance(L"abc", L"def"), 3);
}

TEST(LevenshteinTest, NameVariantExample) {
    // report_1.docx vs report_2.docx — names without ext: "report_1" vs "report_2" = distance 1
    std::wstring a = L"report_1";
    std::wstring b = L"report_2";
    EXPECT_EQ(levenshtein_distance(a, b), 1);
}

TEST(LevenshteinTest, NameVariantExample2) {
    // "report1" vs "report2" — distance 1
    std::wstring a = L"report1";
    std::wstring b = L"report2";
    EXPECT_EQ(levenshtein_distance(a, b), 1);
}

TEST(LevenshteinTest, StringDistance) {
    // "abc" -> "abcd" — distance 1 (insert 'd')
    std::string a = "abc", b = "abcd";
    EXPECT_EQ(levenshtein_distance(a, b), 1);
}

TEST(LevenshteinTest, StringDistance2) {
    // "hello world" -> "hello" — distance 6 (remove ' world')
    std::string a = "hello world", b = "hello";
    EXPECT_EQ(levenshtein_distance(a, b), 6);
}

TEST(LevenshteinTest, LargeStringDistance) {
    // "abcdefg" -> "hijklmn" — distance 7 (all different)
    std::string a = "abcdefg", b = "hijklmn";
    EXPECT_EQ(levenshtein_distance(a, b), 7);
}