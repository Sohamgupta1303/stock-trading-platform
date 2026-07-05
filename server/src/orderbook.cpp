#include "orderbook.h"

#include <algorithm>
#include <iostream>
#include <vector>

#include "market.h"

namespace {
void remove_trader_orders(std::priority_queue<Order>& orders, int trader_id) {
    std::vector<Order> kept;
    kept.reserve(orders.size());
    while (!orders.empty()) {
        Order o = orders.top();
        orders.pop();
        if (o.get_trader_id() != trader_id) {
            kept.push_back(std::move(o));
        }
    }
    for (auto& o : kept) {
        orders.push(std::move(o));
    }
}
}  // namespace

OrderBook::OrderBook(std::string symbol) : symbol_(std::move(symbol)) {}

void OrderBook::add_order(Order order, Market& market) {
    if (order.get_type() == OrderType::BUY) {
        buy_orders_.push(std::move(order));
    } else {
        sell_orders_.push(std::move(order));
    }
    match_orders(market);
}

void OrderBook::match_orders(Market& market) {
    while (!buy_orders_.empty() && !sell_orders_.empty()) {
        if (buy_orders_.top().get_price() < sell_orders_.top().get_price()) {
            break;
        }

        Order buy = buy_orders_.top();
        Order sell = sell_orders_.top();
        buy_orders_.pop();
        sell_orders_.pop();

        int trade_qty = std::min(buy.get_quantity(), sell.get_quantity());
        float trade_price = sell.get_price();  // trade executes at the resting sell order's price

        bool buy_filled = buy.reduce_quantity(trade_qty);
        bool sell_filled = sell.reduce_quantity(trade_qty);

        market.settle_trade(buy.get_trader_id(), sell.get_trader_id(), symbol_, trade_qty, trade_price);

        if (!buy_filled) {
            buy_orders_.push(buy);
        }
        if (!sell_filled) {
            sell_orders_.push(sell);
        }
    }
}

std::pair<std::optional<float>, std::optional<float>> OrderBook::get_best_bid_ask() const {
    std::optional<float> bid;
    std::optional<float> ask;
    if (!buy_orders_.empty()) {
        bid = buy_orders_.top().get_price();
    }
    if (!sell_orders_.empty()) {
        ask = sell_orders_.top().get_price();
    }
    return {bid, ask};
}

void OrderBook::remove_orders_for_trader(int trader_id) {
    remove_trader_orders(buy_orders_, trader_id);
    remove_trader_orders(sell_orders_, trader_id);
}

void OrderBook::print() const {
    auto [bid, ask] = get_best_bid_ask();
    std::cout << symbol_ << " book: " << buy_orders_.size() << " buy order(s)"
              << (bid ? " (best bid $" + std::to_string(*bid) + ")" : "") << ", " << sell_orders_.size()
              << " sell order(s)" << (ask ? " (best ask $" + std::to_string(*ask) + ")" : "") << "\n";
}
