#include "market.h"

#include <gtest/gtest.h>

TEST(MarketTest, AddTraderSetsStartingBalance) {
    Market market;
    market.add_trader(1, 10000.0f);

    ASSERT_NE(market.get_trader(1), nullptr);
    EXPECT_FLOAT_EQ(market.get_trader(1)->get_balance(), 10000.0f);
}

TEST(MarketTest, UnknownTraderInfoReturnsError) {
    Market market;
    EXPECT_NE(market.get_trader_info(999).find("Error"), std::string::npos);
}

TEST(MarketTest, AddStockSeedsMarketMakerLiquidityAroundStartingPrice) {
    Market market;
    market.add_stock("AAPL", "Apple Inc.", 150.0f);

    auto [bid, ask] = market.get_best_bid_ask("AAPL");
    ASSERT_TRUE(bid.has_value());
    ASSERT_TRUE(ask.has_value());
    EXPECT_LT(*bid, 150.0f);
    EXPECT_GT(*ask, 150.0f);
}

TEST(MarketTest, DuplicateStockRegistrationIsIgnored) {
    Market market;
    market.add_stock("AAPL", "Apple Inc.", 150.0f);
    market.add_stock("AAPL", "Apple Inc. Duplicate", 999.0f);

    EXPECT_EQ(market.get_stock("AAPL")->get_name(), "Apple Inc.");
    EXPECT_EQ(market.get_symbols().size(), 1u);
}

TEST(MarketTest, RepriceMarketMakerFollowsNewStockPrice) {
    Market market;
    market.add_stock("AAPL", "Apple Inc.", 150.0f);

    Stock* stock = market.get_stock("AAPL");
    for (int i = 0; i < 200; ++i) {
        stock->update_price();
    }
    market.reprice_market_maker("AAPL");

    auto [bid, ask] = market.get_best_bid_ask("AAPL");
    ASSERT_TRUE(bid.has_value());
    ASSERT_TRUE(ask.has_value());

    float current_price = stock->get_price();
    EXPECT_NEAR(*bid, current_price - Market::kMarketMakerSpread, 1e-4f);
    EXPECT_NEAR(*ask, current_price + Market::kMarketMakerSpread, 1e-4f);
}

TEST(MarketTest, PrintMarketDoesNotCrash) {
    Market market;
    market.add_trader(1, 10000.0f);
    market.add_stock("AAPL", "Apple Inc.", 150.0f);

    testing::internal::CaptureStdout();
    market.print_market();
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_FALSE(output.empty());
}
