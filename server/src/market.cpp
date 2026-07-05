#include "market.h"

#include <iostream>

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
