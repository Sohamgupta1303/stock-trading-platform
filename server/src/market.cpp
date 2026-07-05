#include "market.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>

#include "utils.h"

Market::Market() {
    traders_.emplace(kMarketMakerId, Trader(kMarketMakerStartingBalance));
}

void Market::add_stock(const std::string& symbol, const std::string& name, float initial_price, long market_cap) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (stocks_.count(symbol) > 0) {
        return;
    }

    StockEntry entry{std::make_unique<Stock>(symbol, name, initial_price), OrderBook(symbol), market_cap};
    stocks_.emplace(symbol, std::move(entry));

    // Give the market maker a large share position so it can always sell.
    traders_.at(kMarketMakerId).update_quantity(symbol, kMarketMakerQuoteSize * 10);

    seed_market_maker_quotes(symbol);
}

void Market::add_trader(int id, float initial_balance) {
    std::lock_guard<std::mutex> lock(mutex_);
    traders_.insert_or_assign(id, Trader(initial_balance));
}

void Market::reprice_market_maker(const std::string& symbol) {
    std::lock_guard<std::mutex> lock(mutex_);
    seed_market_maker_quotes(symbol);
}

void Market::seed_market_maker_quotes(const std::string& symbol) {
    auto it = stocks_.find(symbol);
    if (it == stocks_.end()) {
        return;
    }

    float price = it->second.stock->get_price();
    it->second.order_book.remove_orders_for_trader(kMarketMakerId);
    it->second.order_book.add_order(
        Order(kMarketMakerId, kMarketMakerQuoteSize, price - kMarketMakerSpread, OrderType::BUY), *this);
    it->second.order_book.add_order(
        Order(kMarketMakerId, kMarketMakerQuoteSize, price + kMarketMakerSpread, OrderType::SELL), *this);
}

Trader* Market::get_trader(int id) {
    auto it = traders_.find(id);
    return it == traders_.end() ? nullptr : &it->second;
}

Stock* Market::get_stock(const std::string& symbol) {
    auto it = stocks_.find(symbol);
    return it == stocks_.end() ? nullptr : it->second.stock.get();
}

std::string Market::get_trader_info(int id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = traders_.find(id);
    if (it == traders_.end()) {
        return "Error: unknown trader";
    }
    return it->second.get_stocks();
}

std::vector<std::string> Market::get_symbols() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> symbols;
    symbols.reserve(stocks_.size());
    for (const auto& [symbol, entry] : stocks_) {
        symbols.push_back(symbol);
    }
    return symbols;
}

std::pair<std::optional<float>, std::optional<float>> Market::get_best_bid_ask(const std::string& symbol) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = stocks_.find(symbol);
    if (it == stocks_.end()) {
        return {std::nullopt, std::nullopt};
    }
    return it->second.order_book.get_best_bid_ask();
}

void Market::print_market() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::lock_guard<std::mutex> cout_lock(cout_mutex());
    std::cout << "=== Market state ===\n";
    for (const auto& [id, trader] : traders_) {
        if (id == kMarketMakerId) {
            continue;
        }
        std::cout << "Trader " << id << ": " << trader.get_stocks() << "\n";
    }
    for (const auto& [symbol, entry] : stocks_) {
        std::cout << symbol << " ($" << entry.stock->get_price() << ", market cap " << entry.market_cap << "): ";
        entry.order_book.print();
    }
    std::cout << "====================\n";
}

std::string Market::add_order(int trader_id, const std::string& request) {
    std::vector<std::string> tokens = split(request, ':');
    if (tokens.size() != 3 || (tokens[0] != "BUY" && tokens[0] != "SELL")) {
        return "Error: malformed order request (expected BUY:SYMBOL:QTY or SELL:SYMBOL:QTY)";
    }

    const bool is_buy = tokens[0] == "BUY";
    const std::string& symbol = tokens[1];

    int quantity = 0;
    if (!parse_positive_int(tokens[2], quantity)) {
        return "Error: quantity must be a positive whole number";
    }

    std::lock_guard<std::mutex> lock(mutex_);

    auto trader_it = traders_.find(trader_id);
    if (trader_it == traders_.end()) {
        return "Error: unknown trader";
    }
    Trader& trader = trader_it->second;

    auto stock_it = stocks_.find(symbol);
    if (stock_it == stocks_.end()) {
        return "Error: unknown symbol " + symbol;
    }

    const float current_price = stock_it->second.stock->get_price();
    const int qty_before = trader.get_quantity(symbol);

    if (is_buy) {
        // Priced at the current ask so it's guaranteed to cross the market
        // maker's (or any cheaper resting) sell quote; the client never
        // picks its own limit price.
        const float order_price = current_price + kMarketMakerSpread;
        const float max_cost = order_price * static_cast<float>(quantity);
        if (trader.get_balance() < max_cost) {
            return "Error: insufficient balance to buy " + std::to_string(quantity) + " " + symbol;
        }
        stock_it->second.order_book.add_order(Order(trader_id, quantity, order_price, OrderType::BUY), *this);
    } else {
        if (qty_before < quantity) {
            return "Error: insufficient shares to sell " + std::to_string(quantity) + " " + symbol;
        }
        const float order_price = std::max(0.0f, current_price - kMarketMakerSpread);
        stock_it->second.order_book.add_order(Order(trader_id, quantity, order_price, OrderType::SELL), *this);
    }

    const int qty_after = trader.get_quantity(symbol);
    const int filled = is_buy ? qty_after - qty_before : qty_before - qty_after;
    const std::string side = is_buy ? "buy" : "sell";

    if (filled <= 0) {
        return "Order added but not yet filled: " + side + " " + std::to_string(quantity) + " " + symbol;
    }
    if (filled < quantity) {
        return "Order partially filled: " + side + " " + std::to_string(filled) + " of " +
               std::to_string(quantity) + " " + symbol + " (remainder resting)";
    }
    return "Order executed successfully: " + side + " " + std::to_string(quantity) + " " + symbol;
}

std::string Market::recommend_stocks(const std::string& request) {
    std::vector<std::string> tokens = split(request, ':');
    if (tokens.size() != 3 || tokens[0] != "RECOMMEND") {
        return "Error: malformed recommend request (expected RECOMMEND:RISK:COUNT)";
    }

    int risk = 0;
    int count = 0;
    if (!parse_positive_int(tokens[1], risk) || risk < 1 || risk > 3) {
        return "Error: risk tolerance must be 1 (conservative), 2 (moderate), or 3 (aggressive)";
    }
    if (!parse_positive_int(tokens[2], count)) {
        return "Error: count must be a positive whole number";
    }

    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::pair<std::string, float>> scored;
    scored.reserve(stocks_.size());
    for (const auto& [symbol, entry] : stocks_) {
        scored.emplace_back(symbol, entry.stock->volatility());
    }

    if (scored.empty()) {
        return "No stocks available for recommendation";
    }

    if (risk == 1) {
        std::sort(scored.begin(), scored.end(), [](const auto& a, const auto& b) { return a.second < b.second; });
    } else if (risk == 3) {
        std::sort(scored.begin(), scored.end(), [](const auto& a, const auto& b) { return a.second > b.second; });
    } else {
        // "Moderate" reference volatility: the current average across all
        // stocks, so the notion of "moderate" tracks the live market rather
        // than a fixed magic number.
        float sum = 0.0f;
        for (const auto& [symbol, volatility] : scored) {
            sum += volatility;
        }
        const float reference = sum / static_cast<float>(scored.size());
        std::sort(scored.begin(), scored.end(), [reference](const auto& a, const auto& b) {
            return std::fabs(a.second - reference) < std::fabs(b.second - reference);
        });
    }

    const int take = std::min<int>(count, static_cast<int>(scored.size()));
    std::ostringstream oss;
    oss << "Recommended stocks (risk " << risk << "):";
    for (int i = 0; i < take; ++i) {
        oss << " " << scored[i].first;
    }
    return oss.str();
}

void Market::settle_trade(int buyer_id, int seller_id, const std::string& symbol, int quantity, float price) {
    float total = price * static_cast<float>(quantity);

    Trader* buyer = get_trader(buyer_id);
    Trader* seller = get_trader(seller_id);
    if (buyer != nullptr) {
        buyer->update_balance(-total);
        buyer->update_quantity(symbol, quantity);
    }
    if (seller != nullptr) {
        seller->update_balance(total);
        seller->update_quantity(symbol, -quantity);
    }
}
