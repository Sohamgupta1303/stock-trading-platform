#include "trader.h"

#include <sstream>

Trader::Trader(float initial_balance) : balance_(initial_balance) {}

void Trader::update_balance(float amount) {
    balance_ += amount;
}

void Trader::update_quantity(const std::string& symbol, int delta) {
    auto it = stocks_.find(symbol);
    if (it == stocks_.end()) {
        if (delta != 0) {
            stocks_[symbol] = delta;
        }
        return;
    }

    it->second += delta;
    if (it->second <= 0) {
        stocks_.erase(it);
    }
}

float Trader::get_balance() const {
    return balance_;
}

const std::map<std::string, int>& Trader::get_holdings() const {
    return stocks_;
}

int Trader::get_quantity(const std::string& symbol) const {
    auto it = stocks_.find(symbol);
    return it == stocks_.end() ? 0 : it->second;
}

std::string Trader::get_stocks() const {
    std::ostringstream oss;
    oss << "Balance: $" << balance_;
    if (stocks_.empty()) {
        oss << " | Holdings: none";
    } else {
        oss << " | Holdings:";
        for (const auto& [symbol, qty] : stocks_) {
            oss << " " << symbol << ":" << qty;
        }
    }
    return oss.str();
}
