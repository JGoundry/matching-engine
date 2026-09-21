#include <gtest/gtest.h>

#include "OrderPool.h"

// TODO: GTest template type tests to test different size of array
//
namespace testing {

TEST(OrderPoolTest, DefaultTest) {
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

} // namespace testing
