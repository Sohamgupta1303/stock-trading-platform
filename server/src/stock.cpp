#include "stock.h"

#include <cmath>
#include <iostream>
#include <numeric>
#include <random>

namespace {
std::mt19937& rng() {
    thread_local std::mt19937 generator(std::random_device{}());
    return generator;
}
}  // namespace

Stock::Stock(std::string symbol, std::string name, float initial_price)
    : symbol_(std::move(symbol)), name_(std::move(name)), price_(initial_price) {
    price_history_.push_back(price_);
}

void Stock::update_price() {
    std::uniform_real_distribution<float> delta_dist(-0.5f, 0.5f);
    std::lock_guard<std::mutex> lock(mutex_);

    float new_price;
    do {
        new_price = price_ + delta_dist(rng());
    } while (new_price < 0.0f);

    price_ = new_price;
    price_history_.push_back(price_);
}

float Stock::volatility() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (price_history_.size() < 2) {
        return 0.0f;
    }

    float mean = std::accumulate(price_history_.begin(), price_history_.end(), 0.0f) /
                 static_cast<float>(price_history_.size());

    float variance = 0.0f;
    for (float p : price_history_) {
        variance += (p - mean) * (p - mean);
    }
    variance /= static_cast<float>(price_history_.size());

    return std::sqrt(variance);
}

std::string Stock::get_symbol() const {
    return symbol_;
}

std::string Stock::get_name() const {
    return name_;
}

float Stock::get_price() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return price_;
}

std::vector<float> Stock::get_price_history() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return price_history_;
}

void Stock::print() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::cout << symbol_ << " (" << name_ << "): $" << price_ << "\n";
}
