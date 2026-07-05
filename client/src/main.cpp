#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>

#include "utils.h"

// No `using namespace std;` in this file: it calls POSIX socket functions
// (`::connect`, ...) which must not be shadowed by std::bind et al.

namespace {
constexpr int kServerPort = 8000;
constexpr const char* kServerHost = "127.0.0.1";
}  // namespace

int main() {
    int sock_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        std::cerr << "Failed to create socket: " << std::strerror(errno) << "\n";
        return 1;
    }

    sockaddr_in server_address{};
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(kServerPort);
    if (::inet_pton(AF_INET, kServerHost, &server_address.sin_addr) <= 0) {
        std::cerr << "Invalid server address: " << kServerHost << "\n";
        return 1;
    }

    if (::connect(sock_fd, reinterpret_cast<sockaddr*>(&server_address), sizeof(server_address)) < 0) {
        std::cerr << "Failed to connect to " << kServerHost << ":" << kServerPort << ": " << std::strerror(errno)
                   << "\n";
        return 1;
    }

    std::cout << "Connected to stock trading server at " << kServerHost << ":" << kServerPort << "\n";
    std::cout << "Enter commands (GET_STOCKS, BUY:SYMBOL:QTY, SELL:SYMBOL:QTY, RECOMMEND:RISK:COUNT):\n";

    std::string token;
    while (std::cin >> token) {
        if (!send_line(sock_fd, token)) {
            std::cerr << "Connection closed by server.\n";
            break;
        }
        std::string response;
        if (!recv_line(sock_fd, response)) {
            std::cerr << "Connection closed by server.\n";
            break;
        }
        std::cout << response << "\n";
    }

    ::close(sock_fd);
    return 0;
}
