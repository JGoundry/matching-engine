#include <gmock/gmock.h>

#include "OrderPool.h"

#include <algorithm>
#include <array>
#include <cstddef>

namespace test {
namespace {

TEST(OrderPoolTest, ZeroCapacityPoolExhausted) {
  OrderPool<0> pool;
  ASSERT_EQ(pool.capacity(), 0);
  ASSERT_EQ(pool.available(), 0);
  ASSERT_THAT(pool.try_allocate(), ::testing::IsNull());
}

TEST(OrderPoolTest, AllocatesUniqueOrdersUpToCapacityAndRestoresAvailability) {
  constexpr size_t N = 10;
  std::array<Order*, N> orders;
  OrderPool<N> pool;

  ASSERT_EQ(pool.capacity(), N);
  ASSERT_EQ(pool.available(), N);

  // Check we can allocated N orders succesfully and available updates.
  for (size_t i{}; i < N; ++i) {
    Order* order = pool.try_allocate();
    ASSERT_THAT(order, ::testing::NotNull());
    ASSERT_EQ(pool.available(), N - i - 1)
        << "Available counter was not decremented after allocation.";
    orders[i] = order;
  }

  // Check pool is empty and allocation returns nullptr
  ASSERT_EQ(pool.capacity(), N);
  ASSERT_EQ(pool.available(), 0);
  ASSERT_THAT(pool.try_allocate(), ::testing::IsNull());

  // Check orders are unique
  std::ranges::sort(orders);
  ASSERT_EQ(std::ranges::adjacent_find(orders), orders.end())
      << "OrderPool allocated the same order twice.";

  // Check deallocation
  for (size_t i{}; i < N; ++i) {
    pool.deallocate(*orders[i]);
    ASSERT_EQ(pool.available(), i + 1);
  }

  ASSERT_EQ(pool.available(), N);
}

TEST(OrderPoolTest, AllocatedOrderIsEmpty) {
  OrderPool<1> pool;

  // Allocate order and check it is empty
  Order* order = pool.try_allocate();
  ASSERT_THAT(order, ::testing::NotNull());
  ASSERT_EQ(*order, Order{});

  // Update order
  order->id = 1;
  order->next = order;
  order->price = 10;
  order->remaining_qty = 100;

  // Give back to pool
  pool.deallocate(*order);

  // Allocate order and check it is empty
  order = pool.try_allocate();
  ASSERT_THAT(order, ::testing::NotNull());
  ASSERT_EQ(*order, Order{});
}

TEST(OrderPoolTest, OrderPoolReusesDeallocatedOrdersIsLifoOrder) {
  OrderPool<2> pool;
  Order* first_order = pool.try_allocate();
  Order* second_order = pool.try_allocate();
  ASSERT_THAT(first_order, ::testing::NotNull());
  ASSERT_THAT(second_order, ::testing::NotNull());
  pool.deallocate(*first_order);
  pool.deallocate(*second_order);
  ASSERT_EQ(second_order, pool.try_allocate());
  ASSERT_EQ(first_order, pool.try_allocate());
}

TEST(OrderPoolTest, DeathOnDoubleFreeInDebug) {
  OrderPool<1> pool;
  Order* order = pool.try_allocate();
  ASSERT_THAT(order, ::testing::NotNull());
  pool.deallocate(*order);
  ASSERT_DEBUG_DEATH(pool.deallocate(*order), "order was not allocated or was already deallocated");
}

TEST(OrderPoolTest, DeathOnFreeOfOrderNotInPoolInDebug) {
  OrderPool<1> pool;
  Order order;
  ASSERT_DEBUG_DEATH(pool.deallocate(order), "order does not belong to this pool");
}

} // namespace
} // namespace test
