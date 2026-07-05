#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "orderbook.h"
#include "stock.h"
#include "trader.h"

class Market {
public:
    // Reserved trader id for the synthetic market maker that provides
    // standing buy/sell liquidity on every stock.
    static constexpr int kMarketMakerId = -1;
    static constexpr float kMarketMakerSpread = 0.10f;
    static constexpr int kMarketMakerQuoteSize = 1'000'000;
    static constexpr float kMarketMakerStartingBalance = 1'000'000'000.0f;

    Market();

    // Registers a new stock and its order book, and seeds the market
    // maker's initial standing buy/sell quotes around the starting price.
    // Duplicate symbols are ignored.
    void add_stock(const std::string& symbol, const std::string& name, float initial_price, long market_cap = 0);

    // Registers (or resets) a trader with the given starting balance.
    void add_trader(int id, float initial_balance);

    // Re-prices the market maker's standing quotes for `symbol` around its
    // current price: cancels its old resting orders for that stock and adds
    // fresh ones at price +/- kMarketMakerSpread. Called whenever that
    // stock's price updates so resting liquidity never goes stale (see
    // spec's stale-liquidity correctness note).
    void reprice_market_maker(const std::string& symbol);

    // Not internally synchronized: intended for callers that already hold
    // the market lock (Market's own methods) or single-threaded contexts
    // (tests). Concurrent callers should use get_trader_info() instead.
    Trader* get_trader(int id);
    Stock* get_stock(const std::string& symbol);

    // Thread-safe: formatted balance + holdings summary for the given trader.
    std::string get_trader_info(int id);

    std::vector<std::string> get_symbols() const;
    std::pair<std::optional<float>, std::optional<float>> get_best_bid_ask(const std::string& symbol) const;

    void print_market() const;

private:
    friend class OrderBook;

    // Applies the balance/holdings effects of a filled trade. Called by
    // OrderBook::match_orders while the market mutex is already held.
    void settle_trade(int buyer_id, int seller_id, const std::string& symbol, int quantity, float price);

    // Assumes the caller already holds mutex_.
    void seed_market_maker_quotes(const std::string& symbol);

    struct StockEntry {
        std::unique_ptr<Stock> stock;
        OrderBook order_book;
        long market_cap = 0;
    };

    mutable std::mutex mutex_;
    std::map<std::string, StockEntry> stocks_;
    std::map<int, Trader> traders_;
};
