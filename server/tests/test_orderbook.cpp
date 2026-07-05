#include "orderbook.h"

#include <gtest/gtest.h>

#include "market.h"
#include "order.h"

namespace {
constexpr int kBuyerId = 1;
constexpr int kSellerId = 2;
}  // namespace

class OrderBookTest : public ::testing::Test {
protected:
    Market market;
    OrderBook book{"AAPL"};

    void SetUp() override {
        market.add_trader(kBuyerId, 10000.0f);
        market.add_trader(kSellerId, 10000.0f);
        market.get_trader(kSellerId)->update_quantity("AAPL", 50);
    }
};

TEST_F(OrderBookTest, RestingBuyWaitsWithoutMatchingCounterpart) {
    book.add_order(Order(kBuyerId, 10, 100.0f, OrderType::BUY), market);

    auto [bid, ask] = book.get_best_bid_ask();
    ASSERT_TRUE(bid.has_value());
    EXPECT_FALSE(ask.has_value());
    EXPECT_FLOAT_EQ(*bid, 100.0f);
}

TEST_F(OrderBookTest, CrossingOrdersMatchAndSettleAtRestingSellPrice) {
    book.add_order(Order(kBuyerId, 10, 105.0f, OrderType::BUY), market);
    book.add_order(Order(kSellerId, 10, 100.0f, OrderType::SELL), market);

    // Trade must execute at the resting sell order's price (100), not the buy's (105).
    EXPECT_FLOAT_EQ(market.get_trader(kBuyerId)->get_balance(), 10000.0f - 1000.0f);
    EXPECT_EQ(market.get_trader(kBuyerId)->get_quantity("AAPL"), 10);

    EXPECT_FLOAT_EQ(market.get_trader(kSellerId)->get_balance(), 10000.0f + 1000.0f);
    EXPECT_EQ(market.get_trader(kSellerId)->get_quantity("AAPL"), 40);

    auto [bid, ask] = book.get_best_bid_ask();
    EXPECT_FALSE(bid.has_value());
    EXPECT_FALSE(ask.has_value());
}

TEST_F(OrderBookTest, PartialFillLeavesRemainderResting) {
    book.add_order(Order(kBuyerId, 20, 100.0f, OrderType::BUY), market);
    book.add_order(Order(kSellerId, 5, 100.0f, OrderType::SELL), market);

    EXPECT_EQ(market.get_trader(kBuyerId)->get_quantity("AAPL"), 5);

    auto [bid, ask] = book.get_best_bid_ask();
    ASSERT_TRUE(bid.has_value());
    EXPECT_FLOAT_EQ(*bid, 100.0f);
    EXPECT_FALSE(ask.has_value());
}

TEST_F(OrderBookTest, NoMatchWhenBuyPriceBelowSellPrice) {
    book.add_order(Order(kBuyerId, 10, 90.0f, OrderType::BUY), market);
    book.add_order(Order(kSellerId, 10, 100.0f, OrderType::SELL), market);

    auto [bid, ask] = book.get_best_bid_ask();
    ASSERT_TRUE(bid.has_value());
    ASSERT_TRUE(ask.has_value());
    EXPECT_FLOAT_EQ(*bid, 90.0f);
    EXPECT_FLOAT_EQ(*ask, 100.0f);
}

TEST_F(OrderBookTest, RemoveOrdersForTraderDropsOnlyThatTradersOrders) {
    book.add_order(Order(kBuyerId, 10, 90.0f, OrderType::BUY), market);
    market.add_trader(3, 10000.0f);
    book.add_order(Order(3, 5, 95.0f, OrderType::BUY), market);

    book.remove_orders_for_trader(kBuyerId);

    auto [bid, ask] = book.get_best_bid_ask();
    ASSERT_TRUE(bid.has_value());
    EXPECT_FLOAT_EQ(*bid, 95.0f);
}
