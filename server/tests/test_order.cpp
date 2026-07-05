#include "order.h"

#include <gtest/gtest.h>

#include <queue>
#include <thread>

TEST(OrderTest, ReduceQuantityPartialFill) {
    Order o(1, 10, 100.0f, OrderType::BUY);
    EXPECT_FALSE(o.reduce_quantity(4));
    EXPECT_EQ(o.get_quantity(), 6);
}

TEST(OrderTest, ReduceQuantityFullyFilled) {
    Order o(1, 10, 100.0f, OrderType::BUY);
    EXPECT_TRUE(o.reduce_quantity(10));
    EXPECT_EQ(o.get_quantity(), 0);
}

TEST(OrderTest, ReduceQuantityNeverGoesNegative) {
    Order o(1, 10, 100.0f, OrderType::BUY);
    EXPECT_TRUE(o.reduce_quantity(15));
    EXPECT_EQ(o.get_quantity(), 0);
}

TEST(OrderTest, BuyOrderPrefersHigherPrice) {
    Order low(1, 10, 100.0f, OrderType::BUY);
    Order high(2, 10, 105.0f, OrderType::BUY);
    EXPECT_TRUE(low < high);
    EXPECT_FALSE(high < low);
}

TEST(OrderTest, SellOrderPrefersLowerPrice) {
    Order low(1, 10, 100.0f, OrderType::SELL);
    Order high(2, 10, 105.0f, OrderType::SELL);
    EXPECT_TRUE(high < low);
    EXPECT_FALSE(low < high);
}

TEST(OrderTest, BuyOrderTieBrokenByEarlierTimestamp) {
    Order first(1, 10, 100.0f, OrderType::BUY);
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    Order second(2, 10, 100.0f, OrderType::BUY);
    EXPECT_TRUE(second < first);
    EXPECT_FALSE(first < second);
}

TEST(OrderTest, PriorityQueueServesHighestPriceBuyFirst) {
    std::priority_queue<Order> pq;
    pq.push(Order(1, 10, 100.0f, OrderType::BUY));
    pq.push(Order(2, 10, 120.0f, OrderType::BUY));
    pq.push(Order(3, 10, 110.0f, OrderType::BUY));
    EXPECT_FLOAT_EQ(pq.top().get_price(), 120.0f);
}

TEST(OrderTest, PriorityQueueServesLowestPriceSellFirst) {
    std::priority_queue<Order> pq;
    pq.push(Order(1, 10, 100.0f, OrderType::SELL));
    pq.push(Order(2, 10, 120.0f, OrderType::SELL));
    pq.push(Order(3, 10, 90.0f, OrderType::SELL));
    EXPECT_FLOAT_EQ(pq.top().get_price(), 90.0f);
}
