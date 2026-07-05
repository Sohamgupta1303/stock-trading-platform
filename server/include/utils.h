#pragma once

#include <sys/socket.h>

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

// Reads bytes from fd until a '\n' is seen (stripped, along with any '\r')
// or the peer closes the connection. Byte-at-a-time to correctly handle a
// TCP stream where one send() does not necessarily arrive as one recv().
// Returns false once there is no more data to deliver (clean EOF/error with
// nothing buffered).
inline bool recv_line(int fd, std::string& out) {
    out.clear();
    char c;
    while (true) {
        ssize_t n = ::recv(fd, &c, 1, 0);
        if (n <= 0) {
            return !out.empty();
        }
        if (c == '\n') {
            return true;
        }
        if (c != '\r') {
            out.push_back(c);
        }
    }
}

// Sends line followed by a newline delimiter, looping until every byte is
// written (a single send() is not guaranteed to accept the whole buffer).
inline bool send_line(int fd, const std::string& line) {
    std::string framed = line + "\n";
    size_t total_sent = 0;
    while (total_sent < framed.size()) {
        ssize_t n = ::send(fd, framed.data() + total_sent, framed.size() - total_sent, 0);
        if (n <= 0) {
            return false;
        }
        total_sent += static_cast<size_t>(n);
    }
    return true;
}
