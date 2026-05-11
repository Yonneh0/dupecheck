#pragma once
#include <string>
#include <algorithm>
#include <vector>

inline int levenshtein_distance(const std::wstring& a, const std::wstring& b) {
    size_t n = a.length(), m = b.length();
    if (n == 0) return static_cast<int>(m);
    if (m == 0) return static_cast<int>(n);

    std::vector<int> prev(n + 1), curr(n + 1);
    for (size_t i = 0; i <= n; ++i) prev[i] = static_cast<int>(i);
    
    for (size_t j = 1; j <= m; ++j) {
        curr[0] = static_cast<int>(j);
        for (size_t i = 1; i <= n; ++i) {
            int cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
            curr[i] = std::min({curr[i - 1] + 1, prev[i] + 1, prev[i - 1] + cost});
        }
        std::swap(prev, curr);
    }
    
    return prev[n];
}

inline int levenshtein_distance(const std::string& a, const std::string& b) {
    size_t n = a.length(), m = b.length();
    if (n == 0) return static_cast<int>(m);
    if (m == 0) return static_cast<int>(n);

    std::vector<int> prev(n + 1), curr(n + 1);
    for (size_t i = 0; i <= n; ++i) prev[i] = static_cast<int>(i);
    
    for (size_t j = 1; j <= m; ++j) {
        curr[0] = static_cast<int>(j);
        for (size_t i = 1; i <= n; ++i) {
            int cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
            curr[i] = std::min({curr[i - 1] + 1, prev[i] + 1, prev[i - 1] + cost});
        }
        std::swap(prev, curr);
    }
    
    return prev[n];
}
