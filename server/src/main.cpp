#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstring>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "market.h"
#include "utils.h"

// No `using namespace std;` in this file: it calls POSIX socket functions
// (`::bind`, `::accept`, ...) which must not be shadowed by std::bind et al.

namespace {

constexpr int kServerPort = 8000;
constexpr float kStartingBalance = 10000.0f;

std::atomic<bool> running{true};

void handle_sigint(int /*signum*/) {
    // Signal-safe: only flips a flag. The blocking accept() call below is
    // interrupted with EINTR (SA_RESTART is not set), so main() notices
    // promptly and does the actual shutdown/printing there.
    running.store(false);
}

struct StockSeed {
    std::string symbol;
    std::string name;
    float starting_price;
    long market_cap;
};

const std::vector<StockSeed>& initial_stocks() {
    static const std::vector<StockSeed> stocks = {
        {"AAPL", "Apple Inc.", 190.50f, 3'000'000'000'000L},
        {"MSFT", "Microsoft Corporation", 420.30f, 3'100'000'000'000L},
        {"GOOG", "Alphabet Inc.", 165.20f, 2'000'000'000'000L},
        {"AMZN", "Amazon.com Inc.", 178.90f, 1'800'000'000'000L},
        {"TSLA", "Tesla Inc.", 240.10f, 780'000'000'000L},
        {"NVDA", "NVIDIA Corporation", 880.40f, 2'200'000'000'000L},
        {"META", "Meta Platforms Inc.", 480.75f, 1'200'000'000'000L},
        {"NFLX", "Netflix Inc.", 610.20f, 270'000'000'000L},
        {"JPM", "JPMorgan Chase & Co.", 195.60f, 560'000'000'000L},
        {"DIS", "The Walt Disney Company", 112.30f, 205'000'000'000L},
    };
    return stocks;
}

// One per stock: nudges its price on a random 1-15s interval and re-prices
// the market maker's quotes so resting liquidity tracks the new price.
void price_update_loop(Market& market, std::string symbol) {
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> interval_seconds(1, 15);

    while (running.load()) {
        std::this_thread::sleep_for(std::chrono::seconds(interval_seconds(rng)));
        if (!running.load()) {
            return;
        }
        Stock* stock = market.get_stock(symbol);
        if (stock == nullptr) {
            continue;
        }
        stock->update_price();
        market.reprice_market_maker(symbol);
    }
}

void market_printer_loop(Market& market) {
    while (running.load()) {
        std::this_thread::sleep_for(std::chrono::seconds(10));
        if (!running.load()) {
            return;
        }
        market.print_market();
    }
}

std::string dispatch(Market& market, int trader_id, const std::string& request) {
    if (request == "GET_STOCKS") {
        return market.get_trader_info(trader_id);
    }
    if (request.rfind("BUY:", 0) == 0 || request.rfind("SELL:", 0) == 0) {
        return market.add_order(trader_id, request);
    }
    if (request.rfind("RECOMMEND:", 0) == 0) {
        return market.recommend_stocks(request);
    }
    return "Error: unknown command (expected GET_STOCKS, BUY:SYMBOL:QTY, SELL:SYMBOL:QTY, or RECOMMEND:RISK:COUNT)";
}

// One per connected client: registers the trader, then loops reading and
// dispatching requests until the client disconnects.
void handle_client(Market& market, int client_fd, int trader_id) {
    market.add_trader(trader_id, kStartingBalance);
    {
        std::lock_guard<std::mutex> lock(cout_mutex());
        std::cout << "Trader " << trader_id << " connected.\n";
    }

    std::string request;
    while (recv_line(client_fd, request)) {
        if (!request.empty()) {
            std::string response = dispatch(market, trader_id, request);
            if (!send_line(client_fd, response)) {
                break;
            }
        }
    }

    {
        std::lock_guard<std::mutex> lock(cout_mutex());
        std::cout << "Trader " << trader_id << " disconnected.\n";
    }
    ::close(client_fd);
}

}  // namespace

int main() {
    struct sigaction sa {};
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;  // deliberately no SA_RESTART: interrupt the blocking accept()
    sigaction(SIGINT, &sa, nullptr);

    Market market;
    for (const auto& seed : initial_stocks()) {
        market.add_stock(seed.symbol, seed.name, seed.starting_price, seed.market_cap);
    }

    for (const auto& seed : initial_stocks()) {
        std::thread(price_update_loop, std::ref(market), seed.symbol).detach();
    }
    std::thread(market_printer_loop, std::ref(market)).detach();

    int server_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Failed to create socket: " << std::strerror(errno) << "\n";
        return 1;
    }

    int reuse = 1;
    ::setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(kServerPort);

    if (::bind(server_fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        std::cerr << "Failed to bind to port " << kServerPort << ": " << std::strerror(errno) << "\n";
        return 1;
    }
    if (::listen(server_fd, /*backlog=*/16) < 0) {
        std::cerr << "Failed to listen on port " << kServerPort << ": " << std::strerror(errno) << "\n";
        return 1;
    }

    {
        std::lock_guard<std::mutex> lock(cout_mutex());
        std::cout << "Stock trading server listening on port " << kServerPort << "...\n";
    }

    int next_trader_id = 1;
    while (running.load()) {
        sockaddr_in client_address{};
        socklen_t client_len = sizeof(client_address);
        int client_fd = ::accept(server_fd, reinterpret_cast<sockaddr*>(&client_address), &client_len);
        if (client_fd < 0) {
            if (!running.load()) {
                break;
            }
            std::cerr << "accept() failed: " << std::strerror(errno) << "\n";
            continue;
        }
        std::thread(handle_client, std::ref(market), client_fd, next_trader_id++).detach();
    }

    {
        std::lock_guard<std::mutex> lock(cout_mutex());
        std::cout << "\nReceived shutdown signal. Shutting down server...\n";
    }
    ::close(server_fd);
    return 0;
}
