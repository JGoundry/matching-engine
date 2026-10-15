#include <gtest/gtest.h>

#include "OrderPool.h"

namespace test {
namespace {

// TODO: Use template type test to test with N = 0, 10, 100
TEST(OrderPoolTest, DefaultUsage) {
  constexpr std::size_t N = 10;
  OrderPool<N> pool{};

  ASSERT_TRUE(pool.capacity() == N);
  ASSERT_TRUE(pool.available() == N);

  for (size_t i{}; i < N; ++i) {
    Order *order = pool.try_allocate();
    ASSERT_TRUE(order != nullptr);
    ASSERT_TRUE(pool.available() == N - i - 1);
  }

  ASSERT_TRUE(pool.capacity() == N);
  ASSERT_TRUE(pool.available() == 0);
  ASSERT_TRUE(pool.try_allocate() == nullptr);
}

TEST(OrderPoolTest, AllocatedOrderIsEmpty) {
  OrderPool<1> pool{};

  // Allocate order and check it is empty
  Order* order = pool.try_allocate();
  ASSERT_TRUE(order != nullptr);
  ASSERT_TRUE(*order == Order{});

  // Update order
  order->id = 1;
  order->next = order;
  order->price = 10;
  order->qty = 100;

  // Give back to pool
  pool.deallocate(*order);

  // Allocate order and check it is empty
  order = pool.try_allocate();
  ASSERT_TRUE(order != nullptr);
  ASSERT_TRUE(*order == Order{});
}

}
} // namespace testing
