#include "stock.h"

#include <gtest/gtest.h>

TEST(StockTest, InitialState) {
    Stock s("AAPL", "Apple Inc.", 150.0f);
    EXPECT_EQ(s.get_symbol(), "AAPL");
    EXPECT_EQ(s.get_name(), "Apple Inc.");
    EXPECT_FLOAT_EQ(s.get_price(), 150.0f);
    EXPECT_EQ(s.get_price_history().size(), 1u);
}

TEST(StockTest, PriceStaysNonNegativeAfterManyUpdates) {
    Stock s("PENNY", "Penny Stock", 0.1f);
    for (int i = 0; i < 10000; ++i) {
        s.update_price();
        EXPECT_GE(s.get_price(), 0.0f);
    }
}

TEST(StockTest, PriceHistoryGrowsWithEachUpdate) {
    Stock s("AAPL", "Apple Inc.", 150.0f);
    for (int i = 0; i < 5; ++i) {
        s.update_price();
    }
    EXPECT_EQ(s.get_price_history().size(), 6u);
}

TEST(StockTest, VolatilityIsZeroWithNoPriceMovement) {
    Stock s("AAPL", "Apple Inc.", 150.0f);
    EXPECT_FLOAT_EQ(s.volatility(), 0.0f);
}

TEST(StockTest, VolatilityIsPositiveAfterUpdates) {
    Stock s("AAPL", "Apple Inc.", 150.0f);
    for (int i = 0; i < 50; ++i) {
        s.update_price();
    }
    EXPECT_GT(s.volatility(), 0.0f);
}
