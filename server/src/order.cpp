#include "order.h"

#include <algorithm>

Order::Order(int trader_id, int quantity, float price, OrderType type)
    : trader_id_(trader_id),
      quantity_(quantity),
      price_(price),
      type_(type),
      timestamp_(std::chrono::steady_clock::now()) {}

bool Order::reduce_quantity(int amount) {
    quantity_ = std::max(0, quantity_ - amount);
    return quantity_ == 0;
}

int Order::get_trader_id() const {
    return trader_id_;
}

int Order::get_quantity() const {
    return quantity_;
}

float Order::get_price() const {
    return price_;
}

OrderType Order::get_type() const {
    return type_;
}

std::chrono::steady_clock::time_point Order::get_timestamp() const {
    return timestamp_;
}

bool Order::operator<(const Order& other) const {
    if (price_ != other.price_) {
        if (type_ == OrderType::BUY) {
            return price_ < other.price_;   // higher price = higher priority
        }
        return price_ > other.price_;       // lower price = higher priority
    }
    // Tie: whichever order arrived first has higher priority, so the later
    // order compares as "less than" (lower priority).
    return timestamp_ > other.timestamp_;
}
