#pragma once

#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

inline std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delimiter)) {
        tokens.push_back(item);
    }
    return tokens;
}

// Parses s as a strictly positive integer (no sign, no trailing junk).
// Returns false (leaving out untouched) on any malformed input.
inline bool parse_positive_int(const std::string& s, int& out) {
    if (s.empty()) {
        return false;
    }
    try {
        size_t consumed = 0;
        long value = std::stol(s, &consumed);
        if (consumed != s.size() || value <= 0) {
            return false;
        }
        out = static_cast<int>(value);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}
