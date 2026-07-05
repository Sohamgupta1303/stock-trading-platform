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

TEST(MarketAddOrderTest, BuyOrderExecutesAgainstMarketMakerLiquidity) {
    Market market;
    market.add_trader(1, 10000.0f);
    market.add_stock("AAPL", "Apple Inc.", 150.0f);

    std::string response = market.add_order(1, "BUY:AAPL:10");

    EXPECT_NE(response.find("executed successfully"), std::string::npos);
    EXPECT_EQ(market.get_trader(1)->get_quantity("AAPL"), 10);
    EXPECT_LT(market.get_trader(1)->get_balance(), 10000.0f);
}

TEST(MarketAddOrderTest, SellOrderExecutesAgainstMarketMakerLiquidity) {
    Market market;
    market.add_trader(1, 10000.0f);
    market.add_stock("AAPL", "Apple Inc.", 150.0f);
    ASSERT_NE(market.add_order(1, "BUY:AAPL:10").find("executed successfully"), std::string::npos);
    float balance_after_buy = market.get_trader(1)->get_balance();

    std::string response = market.add_order(1, "SELL:AAPL:10");

    EXPECT_NE(response.find("executed successfully"), std::string::npos);
    EXPECT_EQ(market.get_trader(1)->get_quantity("AAPL"), 0);
    EXPECT_GT(market.get_trader(1)->get_balance(), balance_after_buy);
}

TEST(MarketAddOrderTest, MalformedRequestReturnsError) {
    Market market;
    market.add_trader(1, 10000.0f);
    market.add_stock("AAPL", "Apple Inc.", 150.0f);

    EXPECT_NE(market.add_order(1, "BUY:AAPL").find("Error"), std::string::npos);
    EXPECT_NE(market.add_order(1, "HOLD:AAPL:10").find("Error"), std::string::npos);
    EXPECT_NE(market.add_order(1, "BUY:AAPL:notanumber").find("Error"), std::string::npos);
    EXPECT_NE(market.add_order(1, "BUY:AAPL:-5").find("Error"), std::string::npos);
}

TEST(MarketAddOrderTest, UnknownSymbolReturnsError) {
    Market market;
    market.add_trader(1, 10000.0f);

    EXPECT_NE(market.add_order(1, "BUY:ZZZZ:10").find("Error"), std::string::npos);
}

TEST(MarketAddOrderTest, InsufficientBalanceReturnsError) {
    Market market;
    market.add_trader(1, 1.0f);
    market.add_stock("AAPL", "Apple Inc.", 150.0f);

    std::string response = market.add_order(1, "BUY:AAPL:10");

    EXPECT_NE(response.find("Error"), std::string::npos);
    EXPECT_EQ(market.get_trader(1)->get_quantity("AAPL"), 0);
}

TEST(MarketAddOrderTest, InsufficientSharesReturnsError) {
    Market market;
    market.add_trader(1, 10000.0f);
    market.add_stock("AAPL", "Apple Inc.", 150.0f);

    std::string response = market.add_order(1, "SELL:AAPL:10");

    EXPECT_NE(response.find("Error"), std::string::npos);
    EXPECT_FLOAT_EQ(market.get_trader(1)->get_balance(), 10000.0f);
}

TEST(MarketRecommendTest, ConservativeReturnsLowestVolatilityFirst) {
    Market market;
    market.add_stock("CALM", "Calm Corp.", 100.0f);
    market.add_stock("WILD", "Wild Inc.", 100.0f);
    for (int i = 0; i < 2000; ++i) {
        market.get_stock("WILD")->update_price();
    }

    std::string response = market.recommend_stocks("RECOMMEND:1:1");

    EXPECT_NE(response.find("CALM"), std::string::npos);
    EXPECT_EQ(response.find("WILD"), std::string::npos);
}

TEST(MarketRecommendTest, AggressiveReturnsHighestVolatilityFirst) {
    Market market;
    market.add_stock("CALM", "Calm Corp.", 100.0f);
    market.add_stock("WILD", "Wild Inc.", 100.0f);
    for (int i = 0; i < 2000; ++i) {
        market.get_stock("WILD")->update_price();
    }

    std::string response = market.recommend_stocks("RECOMMEND:3:1");

    EXPECT_NE(response.find("WILD"), std::string::npos);
    EXPECT_EQ(response.find("CALM"), std::string::npos);
}

TEST(MarketRecommendTest, InvalidRiskToleranceReturnsError) {
    Market market;
    market.add_stock("AAPL", "Apple Inc.", 150.0f);

    EXPECT_NE(market.recommend_stocks("RECOMMEND:9:1").find("Error"), std::string::npos);
}

TEST(MarketRecommendTest, MalformedRequestReturnsError) {
    Market market;
    market.add_stock("AAPL", "Apple Inc.", 150.0f);

    EXPECT_NE(market.recommend_stocks("RECOMMEND:1").find("Error"), std::string::npos);
    EXPECT_NE(market.recommend_stocks("SUGGEST:1:1").find("Error"), std::string::npos);
}
