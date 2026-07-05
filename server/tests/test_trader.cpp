#include "trader.h"

#include <gtest/gtest.h>

TEST(TraderTest, InitialBalance) {
    Trader t(10000.0f);
    EXPECT_FLOAT_EQ(t.get_balance(), 10000.0f);
    EXPECT_TRUE(t.get_holdings().empty());
}

TEST(TraderTest, UpdateBalancePositiveAndNegative) {
    Trader t(1000.0f);
    t.update_balance(-200.0f);
    EXPECT_FLOAT_EQ(t.get_balance(), 800.0f);
    t.update_balance(50.0f);
    EXPECT_FLOAT_EQ(t.get_balance(), 850.0f);
}

TEST(TraderTest, UpdateQuantityAddsNewHolding) {
    Trader t(1000.0f);
    t.update_quantity("AAPL", 10);
    EXPECT_EQ(t.get_quantity("AAPL"), 10);
}

TEST(TraderTest, UpdateQuantityAccumulates) {
    Trader t(1000.0f);
    t.update_quantity("AAPL", 10);
    t.update_quantity("AAPL", 5);
    EXPECT_EQ(t.get_quantity("AAPL"), 15);
}

TEST(TraderTest, UpdateQuantityRemovesSymbolWhenItHitsZero) {
    Trader t(1000.0f);
    t.update_quantity("AAPL", 10);
    t.update_quantity("AAPL", -10);
    EXPECT_EQ(t.get_quantity("AAPL"), 0);
    EXPECT_EQ(t.get_holdings().count("AAPL"), 0u);
}

TEST(TraderTest, GetStocksSummaryReflectsBalanceAndHoldings) {
    Trader t(500.0f);
    t.update_quantity("AAPL", 3);
    std::string summary = t.get_stocks();
    EXPECT_NE(summary.find("500"), std::string::npos);
    EXPECT_NE(summary.find("AAPL"), std::string::npos);
    EXPECT_NE(summary.find("3"), std::string::npos);
}

TEST(TraderTest, GetStocksSummaryWithNoHoldings) {
    Trader t(100.0f);
    std::string summary = t.get_stocks();
    EXPECT_NE(summary.find("none"), std::string::npos);
}
