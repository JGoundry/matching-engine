#include <gmock/gmock.h>

#include "OrderBook.h"

namespace test {
namespace {

[[nodiscard]] Order create_order(OrderId id, Price price, Quantity remaining_qty) {
  return Order{.id=id, .price=price, .remaining_qty=remaining_qty, .next=nullptr};
}

[[nodiscard]] auto best_level(auto& book, Side side) {
  return side == Side::Buy ? book.best_bid_level() : book.best_ask_level();
}

[[nodiscard]] std::string side_name(const ::testing::TestParamInfo<Side>& info) {
  switch (info.param) {
    case Side::None:
      return "None";
    case Side::Buy:
      return "Buy";
    case Side::Sell:
      return "Sell";
  }
  assert(false && "missing side name");
  return "ERROR: missing side in side_name function";
}

[[nodiscard]] Side opposite_side(Side side) {
  switch (side) {
    case Side::Buy:
      return Side::Sell;
    case Side::Sell:
      return Side::Buy;
    default:
      assert(false && "only Buy and Sell have opposites");
      return Side::None;
  }
}

class OrderBookSideTest : public ::testing::TestWithParam<Side> {};
class OrderBookIsRestingSideTest : public OrderBookSideTest {};
class OrderBookAddRestingSideTest : public OrderBookSideTest {};

TEST(OrderBookIsRestingTest, DeathWhenOrderSideIsNoneInDebug) {
  OrderBook<1> book;
  Order order = create_order(0, 10, 10);
  ASSERT_DEBUG_DEATH(static_cast<void>(book.is_resting(order, Side::None)), "side must not be Side::None");
}

TEST_P(OrderBookAddRestingSideTest, DeathWhenOrderIsAlreadyRestingOnOppositeSideInDebug) {
  OrderBook<1> book;
  const Side side = GetParam();
  Order order = create_order(0, 5, 10);
  ASSERT_TRUE(book.try_add_resting(order, opposite_side(side)));
  ASSERT_DEBUG_DEATH(static_cast<void>(book.try_add_resting(order, side)), "order must not be in book already");
}

TEST_P(OrderBookIsRestingSideTest, ReturnsFalseWhenOrderIsNotInBookAndTrueWhenInBook) {
  OrderBook<1> book;
  const Side side = GetParam();
  Order order = create_order(0, 10, 10);
  ASSERT_FALSE(book.is_resting(order, side)) << "should return false when order not at rest";
  ASSERT_TRUE(book.try_add_resting(order, side));
  ASSERT_TRUE(book.is_resting(order, side)) << "should return true when at rest";
}

TEST_P(OrderBookIsRestingSideTest, ReturnsTrueWhenOrderIsDeepInPriceLevelList) {
  OrderBook<3> book;
  const Side side = GetParam();
  Order first_order = create_order(0, 10, 10);
  Order second_order = create_order(1, 10, 10);
  Order third_order = create_order(2, 10, 10);
  ASSERT_TRUE(book.try_add_resting(first_order, side));
  ASSERT_TRUE(book.try_add_resting(second_order, side));
  ASSERT_TRUE(book.try_add_resting(third_order, side));
  ASSERT_TRUE(book.is_resting(first_order, side));
  ASSERT_TRUE(book.is_resting(second_order, side));
  ASSERT_TRUE(book.is_resting(third_order, side));
}

TEST_P(OrderBookIsRestingSideTest, ReturnsFalseForDifferentOrderAtExistingPriceLevel) {
  OrderBook<1> book;
  const Side side = GetParam();
  Order first_order = create_order(0, 10, 10);
  Order second_order = create_order(1, 10, 10);
  ASSERT_TRUE(book.try_add_resting(first_order, side));
  ASSERT_FALSE(book.is_resting(second_order, side));
}

INSTANTIATE_TEST_SUITE_P(
  BothSides,
  OrderBookIsRestingSideTest,
  ::testing::Values(Side::Buy, Side::Sell),
  side_name
);

TEST(OrderBookAddRestingTest, DeathWhenOrderRemainingQtyIsZeroInDebug) {
  OrderBook<1> book;
  Order first_order = create_order(0, 5, 0);
  ASSERT_DEBUG_DEATH(static_cast<void>(book.try_add_resting(first_order, Side::Buy)), "remaining_qty must be greater than zero");
}

TEST(OrderBookAddRestingTest, DeathWhenOrderIsInBookAlreadyInDebug) {
  OrderBook<1> book;
  Order first_order = create_order(0, 5, 10);
  ASSERT_TRUE(book.try_add_resting(first_order, Side::Buy));
  ASSERT_DEBUG_DEATH(static_cast<void>(book.try_add_resting(first_order, Side::Buy)), "order must not be in book already");
}

TEST(OrderBookAddRestingTest, DeathWhenOrderSideIsNoneInDebug) {
  OrderBook<1> book;
  Order first_order = create_order(0, 5, 10);
  ASSERT_DEBUG_DEATH(static_cast<void>(book.try_add_resting(first_order, Side::None)), "side must not be Side::None");
}

TEST_P(OrderBookAddRestingSideTest, AcceptsOrderWhenPriceLevelExistsAndAtCapacity) {
  OrderBook<1> book;

  const Side side = GetParam();
  const Price price = 5;
  Order first_order = create_order(0, price, 100);
  Order second_order = create_order(1, price, 320);

  ASSERT_THAT(best_level(book, side), ::testing::IsNull()) << "book should return nullptr for price level when empty";

  ASSERT_TRUE(book.try_add_resting(first_order, side)) << "book should accept order at new price level when there is capcity";
  ASSERT_TRUE(book.try_add_resting(second_order, side)) << "book should accept new order when price level exists and at capcity";

  // Check they were added correctly
  const auto* level = best_level(book, side);
  ASSERT_THAT(level, ::testing::NotNull()) << "book should return a price level when order exists";
  ASSERT_EQ(level->price, price) << "price at level should match the order we inserted";
  ASSERT_EQ(level->remaining_qty, first_order.remaining_qty + second_order.remaining_qty) << "price level quantity should be sum of all quantity at level";

  // Validate price level linked list (check FIFO)
  ASSERT_EQ(level->head, &first_order) << "head should be first order that went in";
  ASSERT_EQ(level->head->next, &second_order) << "head should point to the next order in list";
  ASSERT_EQ(level->tail, &second_order) << "tail should be last order that went in";
  ASSERT_THAT(level->tail->next, ::testing::IsNull()) << "tail->next should always be null to mark end of list";
}

TEST_P(OrderBookAddRestingSideTest, RejectsOrderWhenPriceLevelDoesNotExistAndAtCapacity) {
  OrderBook<1> book;

  const Side side = GetParam();

  Order first_order = create_order(0, 5, 100);
  Order second_order = create_order(1, 7, 320);
  Order second_order_copy = second_order;

  ASSERT_TRUE(book.try_add_resting(first_order, side)) << "book should allow adding resting when not at capacity";
  ASSERT_FALSE(book.try_add_resting(second_order, side)) << "book should reject order at new price level when at capacity";

  ASSERT_EQ(second_order, second_order_copy) << "book should not mutate rejected order";
  ASSERT_FALSE(book.is_resting(second_order, side)) << "rejected order should not be in book";
}

TEST_P(OrderBookAddRestingSideTest, BookFollowsPricePriority) {
  OrderBook<4> book;

  const Side side = GetParam();

  Order first_order = create_order(0, 100, 100);
  Order second_order = create_order(1, 10, 100); // lowest price
  Order third_order = create_order(2, 200, 100); // highest price
  Order fourth_order = create_order(3, 110, 100);

  Order& best_order = (side == Side::Buy ? third_order : second_order);

  ASSERT_TRUE(book.try_add_resting(first_order, side));
  ASSERT_TRUE(book.try_add_resting(second_order, side));
  ASSERT_TRUE(book.try_add_resting(third_order, side));
  ASSERT_TRUE(book.try_add_resting(fourth_order, side));

  const auto* level = best_level(book, side);
  ASSERT_THAT(level, ::testing::NotNull());

  ASSERT_EQ(level->head, &best_order);
  ASSERT_EQ(level->price, best_order.price);
  ASSERT_EQ(level->remaining_qty, best_order.remaining_qty);
}

INSTANTIATE_TEST_SUITE_P(
  BothSides,
  OrderBookAddRestingSideTest,
  ::testing::Values(Side::Buy, Side::Sell),
  side_name
);

TEST(OrderBookBestBidLevel, ReturnsBestLevelWhenMultipleInBook) {
  OrderBook<3> book;
  const Side side = Side::Buy;
  Order first_order = create_order(0, 101, 100);
  Order second_order = create_order(1, 200, 100); // best bid
  Order third_order = create_order(2, 100, 100);
  ASSERT_TRUE(book.try_add_resting(first_order, side));
  ASSERT_TRUE(book.try_add_resting(second_order, side));
  ASSERT_TRUE(book.try_add_resting(third_order, side));
  ASSERT_TRUE(book.is_resting(first_order, side));
  ASSERT_TRUE(book.is_resting(second_order, side));
  ASSERT_TRUE(book.is_resting(third_order, side));

  const auto* level = book.best_bid_level();
  ASSERT_THAT(level, ::testing::NotNull());
  ASSERT_EQ(level->price, second_order.price);
}

TEST(OrderBookBestAskLevel, ReturnsBestLevelWhenMultipleInBook) {
  OrderBook<3> book;
  const Side side = Side::Sell;
  Order first_order = create_order(0, 101, 100);
  Order second_order = create_order(1, 10, 100); // best ask
  Order third_order = create_order(2, 100, 100);
  ASSERT_TRUE(book.try_add_resting(first_order, side));
  ASSERT_TRUE(book.try_add_resting(second_order, side));
  ASSERT_TRUE(book.try_add_resting(third_order, side));
  ASSERT_TRUE(book.is_resting(first_order, side));
  ASSERT_TRUE(book.is_resting(second_order, side));
  ASSERT_TRUE(book.is_resting(third_order, side));

  const auto* level = book.best_ask_level();
  ASSERT_THAT(level, ::testing::NotNull());
  ASSERT_EQ(level->price, second_order.price);
}

}
}