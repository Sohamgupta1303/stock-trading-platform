#pragma once

#include <mutex>
#include <string>
#include <vector>

class Stock {
public:
    Stock(std::string symbol, std::string name, float initial_price);

    // Random-walk step: nudge the price by a small random delta, rejecting
    // deltas that would push the price below 0, and record the new price.
    void update_price();

    // Standard deviation of price_history_, used as a risk/volatility score.
    float volatility() const;

    std::string get_symbol() const;
    std::string get_name() const;
    float get_price() const;
    std::vector<float> get_price_history() const;

    void print() const;

private:
    std::string symbol_;
    std::string name_;
    float price_;
    std::vector<float> price_history_;
    mutable std::mutex mutex_;
};
