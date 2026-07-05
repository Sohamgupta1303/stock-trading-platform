#pragma once

#include <optional>
#include <queue>
#include <string>
#include <utility>

#include "order.h"

// Forward declaration only: the full Market definition is needed just in
// orderbook.cpp. Market's own header includes this one, so including
// market.h here would create a circular #include.
class Market;

class OrderBook {
public:
    explicit OrderBook(std::string symbol);

    // Pushes the order onto the correct side, then attempts to match.
    // Assumes the caller already holds Market's mutex.
    void add_order(Order order, Market& market);

    // While the best resting buy price >= the best resting sell price,
    // executes trades (at the resting sell order's price) until no crosses
    // remain. Assumes the caller already holds Market's mutex.
    void match_orders(Market& market);

    // {best_bid, best_ask}; either is empty if that side has no resting orders.
    std::pair<std::optional<float>, std::optional<float>> get_best_bid_ask() const;

    // Drops all resting orders placed by trader_id. Used to re-price the
    // market maker's standing quotes whenever a stock's price updates, so
    // liquidity never goes stale (see spec's stale-liquidity correctness note).
    void remove_orders_for_trader(int trader_id);

    void print() const;

private:
    std::string symbol_;
    std::priority_queue<Order> buy_orders_;
    std::priority_queue<Order> sell_orders_;
};
