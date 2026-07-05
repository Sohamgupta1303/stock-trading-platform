#pragma once

#include <chrono>

enum class OrderType { BUY, SELL };

class Order {
public:
    Order(int trader_id, int quantity, float price, OrderType type);

    // Partially fill the order; returns true once it's fully filled (qty == 0).
    bool reduce_quantity(int amount);

    int get_trader_id() const;
    int get_quantity() const;
    float get_price() const;
    OrderType get_type() const;
    std::chrono::steady_clock::time_point get_timestamp() const;

    // Price-time priority: BUY orders prioritize higher price (max-heap),
    // SELL orders prioritize lower price (min-heap), ties broken by whichever
    // order arrived first. A single std::priority_queue<Order> per side works
    // correctly because this operator branches on order_type.
    bool operator<(const Order& other) const;

private:
    int trader_id_;
    int quantity_;
    float price_;
    OrderType type_;
    std::chrono::steady_clock::time_point timestamp_;
};
