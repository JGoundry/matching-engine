#include <gmock/gmock.h>

#include "OrderBook.h"

namespace test {
namespace {

[[nodiscard]] Order create_order(OrderId id, Price price, Quantity remaining_qty) {
  return Order{.id = id, .price = price, .remaining_qty = remaining_qty, .next = nullptr};
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
class OrderBookReduceRestingSideTest : public OrderBookSideTest {};
class OrderBookReduceRestingThreeOrdersSideTest : public OrderBookSideTest {
protected:
  static constexpr Price price = 10;
  static constexpr Quantity initial_qty = 10;

  OrderBook<1> book;
  Side side{};
  Order first_order = create_order(0, price, initial_qty);
  Order second_order = create_order(1, price, initial_qty);
  Order third_order = create_order(2, price, initial_qty);

  void SetUp() override {
    side = GetParam();
    ASSERT_TRUE(book.try_add_resting(first_order, side));
    ASSERT_TRUE(book.try_add_resting(second_order, side));
    ASSERT_TRUE(book.try_add_resting(third_order, side));

    // Assert orders were added correctly
    const auto* level = best_level(book, side);
    ASSERT_THAT(level, ::testing::NotNull());
    ASSERT_EQ(level->price, price);
    ASSERT_EQ(level->head, &first_order);
    ASSERT_EQ(first_order.next, &second_order);
    ASSERT_EQ(second_order.next, &third_order);
    ASSERT_EQ(level->tail, &third_order);
    ASSERT_THAT(third_order.next, ::testing::IsNull());
    ASSERT_EQ(level->remaining_qty, initial_qty * 3);
  }
};

TEST(OrderBookIsRestingTest, DeathWhenOrderSideIsNoneInDebug) {
  OrderBook<1> book;
  Order order = create_order(0, 10, 10);
  ASSERT_DEBUG_DEATH(static_cast<void>(book.is_resting(order, Side::None)),
                     "side must not be Side::None");
}

TEST_P(OrderBookAddRestingSideTest, DeathWhenOrderIsAlreadyRestingOnOppositeSideInDebug) {
  OrderBook<1> book;
  const Side side = GetParam();
  Order order = create_order(0, 5, 10);
  ASSERT_TRUE(book.try_add_resting(order, opposite_side(side)));
  ASSERT_DEBUG_DEATH(static_cast<void>(book.try_add_resting(order, side)),
                     "order must not be in book already");
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

INSTANTIATE_TEST_SUITE_P(BothSides, OrderBookIsRestingSideTest,
                         ::testing::Values(Side::Buy, Side::Sell), side_name);

TEST(OrderBookAddRestingTest, DeathWhenOrderRemainingQtyIsZeroInDebug) {
  OrderBook<1> book;
  Order first_order = create_order(0, 5, 0);
  ASSERT_DEBUG_DEATH(static_cast<void>(book.try_add_resting(first_order, Side::Buy)),
                     "remaining_qty must be greater than zero");
}

TEST(OrderBookAddRestingTest, DeathWhenOrderIsInBookAlreadyInDebug) {
  OrderBook<1> book;
  Order first_order = create_order(0, 5, 10);
  ASSERT_TRUE(book.try_add_resting(first_order, Side::Buy));
  ASSERT_DEBUG_DEATH(static_cast<void>(book.try_add_resting(first_order, Side::Buy)),
                     "order must not be in book already");
}

TEST(OrderBookAddRestingTest, DeathWhenOrderSideIsNoneInDebug) {
  OrderBook<1> book;
  Order first_order = create_order(0, 5, 10);
  ASSERT_DEBUG_DEATH(static_cast<void>(book.try_add_resting(first_order, Side::None)),
                     "side must not be Side::None");
}

TEST_P(OrderBookAddRestingSideTest, AcceptsOrderWhenPriceLevelExistsAndAtCapacity) {
  OrderBook<1> book;

  const Side side = GetParam();
  const Price price = 5;
  Order first_order = create_order(0, price, 100);
  Order second_order = create_order(1, price, 320);

  ASSERT_THAT(best_level(book, side), ::testing::IsNull())
      << "book should return nullptr for price level when empty";

  ASSERT_TRUE(book.try_add_resting(first_order, side))
      << "book should accept order at new price level when there is capcity";
  ASSERT_TRUE(book.try_add_resting(second_order, side))
      << "book should accept new order when price level exists and at capcity";

  // Check they were added correctly
  const auto* level = best_level(book, side);
  ASSERT_THAT(level, ::testing::NotNull()) << "book should return a price level when order exists";
  ASSERT_EQ(level->price, price) << "price at level should match the order we inserted";
  ASSERT_EQ(level->remaining_qty, first_order.remaining_qty + second_order.remaining_qty)
      << "price level quantity should be sum of all quantity at level";

  // Validate price level linked list (check FIFO)
  ASSERT_EQ(level->head, &first_order) << "head should be first order that went in";
  ASSERT_EQ(level->head->next, &second_order) << "head should point to the next order in list";
  ASSERT_EQ(level->tail, &second_order) << "tail should be last order that went in";
  ASSERT_THAT(level->tail->next, ::testing::IsNull())
      << "tail->next should always be null to mark end of list";
}

TEST_P(OrderBookAddRestingSideTest, RejectsOrderWhenPriceLevelDoesNotExistAndAtCapacity) {
  OrderBook<1> book;

  const Side side = GetParam();

  Order first_order = create_order(0, 5, 100);
  Order second_order = create_order(1, 7, 320);
  Order second_order_copy = second_order;

  ASSERT_TRUE(book.try_add_resting(first_order, side))
      << "book should allow adding resting when not at capacity";
  ASSERT_FALSE(book.try_add_resting(second_order, side))
      << "book should reject order at new price level when at capacity";

  ASSERT_EQ(second_order, second_order_copy) << "book should not mutate rejected order";
  ASSERT_FALSE(book.is_resting(second_order, side)) << "rejected order should not be in book";
}

TEST_P(OrderBookAddRestingSideTest, RejectsOrderWhenQtyWouldOverflowLevelQty) {
  OrderBook<1> book;
  const Side side = GetParam();
  const auto max_qty = std::numeric_limits<Quantity>::max();
  Order first_order = create_order(0, 5, max_qty);
  Order second_order = create_order(1, 5, 1);
  Order second_order_copy = create_order(1, 5, 1);

  ASSERT_TRUE(book.try_add_resting(first_order, side));
  ASSERT_FALSE(book.try_add_resting(second_order, side))
      << "book should reject order when it would overflow level quantity";

  ASSERT_EQ(second_order, second_order_copy) << "book should not mutate rejected order";
  ASSERT_FALSE(book.is_resting(second_order, side)) << "rejected order should not be in book";

  const auto* level = best_level(book, side);
  ASSERT_THAT(level, ::testing::NotNull());
  ASSERT_EQ(level->remaining_qty, max_qty);
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

INSTANTIATE_TEST_SUITE_P(BothSides, OrderBookAddRestingSideTest,
                         ::testing::Values(Side::Buy, Side::Sell), side_name);

TEST(OrderBookBestBidLevelTest, ReturnsBestLevelWhenMultipleInBook) {
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

TEST(OrderBookBestAskLevelTest, ReturnsBestLevelWhenMultipleInBook) {
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

TEST_P(OrderBookReduceRestingSideTest, DeathWhenOrderSideIsNoneInDebug) {
  OrderBook<1> book;
  const Side side = GetParam();
  Order order = create_order(0, 10, 10);
  ASSERT_TRUE(book.try_add_resting(order, side));
  ASSERT_DEBUG_DEATH(static_cast<void>(book.reduce_resting(order, Side::None, 1)),
                     "side must not be Side::None");
}

TEST_P(OrderBookReduceRestingSideTest, DeathWhenReductionQtyIsZeroInDebug) {
  OrderBook<1> book;
  const Side side = GetParam();
  Order order = create_order(0, 10, 10);
  ASSERT_TRUE(book.try_add_resting(order, side));
  ASSERT_DEBUG_DEATH(static_cast<void>(book.reduce_resting(order, side, 0)),
                     "reduction_qty must be greater than zero");
}

TEST_P(OrderBookReduceRestingSideTest, DeathWhenRemainingQtyLessThanReductionQty) {
  OrderBook<1> book;
  const Side side = GetParam();
  Order order = create_order(0, 10, 10);
  ASSERT_TRUE(book.try_add_resting(order, side));
  ASSERT_DEBUG_DEATH(static_cast<void>(book.reduce_resting(order, side, 20)),
                     "order.remaining_qty must be greater than or equal to reduction amount");
}

TEST_P(OrderBookReduceRestingSideTest, DeathWhenOrderIsNotRestingAtGivenSide) {
  OrderBook<1> book;
  const Side side = GetParam();
  Order order = create_order(0, 10, 10);
  ASSERT_DEBUG_DEATH(static_cast<void>(book.reduce_resting(order, side, 5)),
                     "order to be reduced must be resting in given side");
  ASSERT_TRUE(book.try_add_resting(order, opposite_side(side)));
  ASSERT_DEBUG_DEATH(static_cast<void>(book.reduce_resting(order, side, 5)),
                     "order to be reduced must be resting in given side");
}

TEST_P(OrderBookReduceRestingThreeOrdersSideTest, OrderAndLevelReducedWhenNotFullyFilled) {
  const Quantity reduction_qty = 5;

  const auto* level = best_level(book, side);
  ASSERT_THAT(level, ::testing::NotNull());
  ASSERT_EQ(level->price, price);
  ASSERT_EQ(level->head, &first_order);
  ASSERT_EQ(level->tail, &third_order);
  ASSERT_EQ(level->remaining_qty, initial_qty * 3);

  ASSERT_FALSE(book.reduce_resting(first_order, side, reduction_qty));
  ASSERT_EQ(first_order.remaining_qty, initial_qty - reduction_qty);
  level = best_level(book, side);
  ASSERT_THAT(level, ::testing::NotNull());
  ASSERT_EQ(level->price, price);
  ASSERT_EQ(level->head, &first_order);
  ASSERT_EQ(first_order.next, &second_order);
  ASSERT_EQ(level->tail, &third_order);
  ASSERT_THAT(third_order.next, ::testing::IsNull());
  ASSERT_EQ(level->remaining_qty, initial_qty * 3 - reduction_qty);
  ASSERT_EQ(level->remaining_qty,
            first_order.remaining_qty + second_order.remaining_qty + third_order.remaining_qty);
  ASSERT_TRUE(book.is_resting(first_order, side));
  ASSERT_TRUE(book.is_resting(second_order, side));
  ASSERT_TRUE(book.is_resting(third_order, side));
}

TEST_P(OrderBookReduceRestingThreeOrdersSideTest, OrderRemovedWhenFullyFilledAndAtHead) {
  ASSERT_TRUE(book.reduce_resting(first_order, side, initial_qty))
      << "should return true when order fully filled";
  ASSERT_EQ(first_order.remaining_qty, 0);
  const auto* level = best_level(book, side);
  ASSERT_THAT(level, ::testing::NotNull())
      << "level should still exist after order removed when orders remain";
  ASSERT_EQ(level->head, &second_order);
  ASSERT_EQ(level->head->next, &third_order);
  ASSERT_EQ(level->tail, &third_order);
  ASSERT_THAT(level->tail->next, ::testing::IsNull());
  ASSERT_EQ(level->remaining_qty, initial_qty * 2);
  ASSERT_EQ(level->remaining_qty, second_order.remaining_qty + third_order.remaining_qty);
  ASSERT_FALSE(book.is_resting(first_order, side));
  ASSERT_TRUE(book.is_resting(second_order, side));
  ASSERT_TRUE(book.is_resting(third_order, side));
}

TEST_P(OrderBookReduceRestingThreeOrdersSideTest, OrderRemovedWhenFullyFilledAndAtMiddle) {
  ASSERT_TRUE(book.reduce_resting(second_order, side, initial_qty))
      << "should return true when order fully filled";
  ASSERT_EQ(second_order.remaining_qty, 0);
  const auto* level = best_level(book, side);
  ASSERT_THAT(level, ::testing::NotNull())
      << "level should still exist after order removed when other orders still remain";
  ASSERT_EQ(level->head, &first_order);
  ASSERT_EQ(level->head->next, &third_order);
  ASSERT_EQ(level->tail, &third_order);
  ASSERT_THAT(level->tail->next, ::testing::IsNull());
  ASSERT_EQ(level->remaining_qty, initial_qty * 2);
  ASSERT_EQ(level->remaining_qty, first_order.remaining_qty + third_order.remaining_qty);
  ASSERT_TRUE(book.is_resting(first_order, side));
  ASSERT_FALSE(book.is_resting(second_order, side));
  ASSERT_TRUE(book.is_resting(third_order, side));
}

TEST_P(OrderBookReduceRestingThreeOrdersSideTest, OrderRemovedWhenFullyFilledAndAtTail) {
  ASSERT_TRUE(book.reduce_resting(third_order, side, initial_qty));
  ASSERT_EQ(third_order.remaining_qty, 0);

  const auto* level = best_level(book, side);
  ASSERT_THAT(level, ::testing::NotNull());
  ASSERT_EQ(level->head, &first_order);
  ASSERT_EQ(first_order.next, &second_order);
  ASSERT_EQ(level->tail, &second_order);
  ASSERT_THAT(second_order.next, ::testing::IsNull());
  ASSERT_EQ(level->remaining_qty, initial_qty * 2);
  ASSERT_EQ(level->remaining_qty, first_order.remaining_qty + second_order.remaining_qty);
  ASSERT_TRUE(book.is_resting(first_order, side));
  ASSERT_TRUE(book.is_resting(second_order, side));
  ASSERT_FALSE(book.is_resting(third_order, side));
}

TEST_P(OrderBookReduceRestingSideTest, OrderAndLevelRemovedWhenFullyFilledAndOnlyOrderAtLevel) {
  OrderBook<1> book;
  const Side side = GetParam();
  Order order = create_order(0, 10, 10);

  ASSERT_TRUE(book.try_add_resting(order, side));
  ASSERT_TRUE(book.reduce_resting(order, side, order.remaining_qty));

  ASSERT_EQ(order.remaining_qty, 0);
  ASSERT_THAT(best_level(book, side), ::testing::IsNull());
  ASSERT_FALSE(book.is_resting(order, side));
}

TEST_P(OrderBookReduceRestingThreeOrdersSideTest,
       TailIsUpdatedWhenTailOrderIsFullyFilledAndNewOrderIsAdded) {
  ASSERT_TRUE(book.reduce_resting(third_order, side, initial_qty));

  Order fourth_order = create_order(3, price, initial_qty);
  ASSERT_TRUE(book.try_add_resting(fourth_order, side));

  const auto* level = best_level(book, side);
  ASSERT_THAT(level, ::testing::NotNull());
  ASSERT_EQ(level->head, &first_order);
  ASSERT_EQ(first_order.next, &second_order);
  ASSERT_EQ(second_order.next, &fourth_order);
  ASSERT_EQ(level->tail, &fourth_order);
  ASSERT_THAT(fourth_order.next, ::testing::IsNull());
  ASSERT_EQ(level->remaining_qty, initial_qty * 3);
  ASSERT_TRUE(book.is_resting(first_order, side));
  ASSERT_TRUE(book.is_resting(second_order, side));
  ASSERT_FALSE(book.is_resting(third_order, side));
  ASSERT_TRUE(book.is_resting(fourth_order, side));
}

TEST_P(OrderBookReduceRestingThreeOrdersSideTest,
       OtherOrdersRemainRestingWhenAnOrderIsFullyFilled) {
  ASSERT_TRUE(book.reduce_resting(second_order, side, initial_qty));

  ASSERT_EQ(first_order.remaining_qty, initial_qty);
  ASSERT_EQ(third_order.remaining_qty, initial_qty);
  ASSERT_TRUE(book.is_resting(first_order, side));
  ASSERT_FALSE(book.is_resting(second_order, side));
  ASSERT_TRUE(book.is_resting(third_order, side));
}

TEST_P(OrderBookReduceRestingSideTest, RemovingPriceLevelPreservesOtherPriceLevels) {
  OrderBook<2> book;
  const Side side = GetParam();
  Order low_price_order = create_order(0, 10, 10);
  Order high_price_order = create_order(1, 20, 10);
  Order& removed_order = (side == Side::Buy ? high_price_order : low_price_order);
  Order& remaining_order = (side == Side::Buy ? low_price_order : high_price_order);

  ASSERT_TRUE(book.try_add_resting(low_price_order, side));
  ASSERT_TRUE(book.try_add_resting(high_price_order, side));
  ASSERT_TRUE(book.reduce_resting(removed_order, side, removed_order.remaining_qty));

  const auto* level = best_level(book, side);
  ASSERT_THAT(level, ::testing::NotNull());
  ASSERT_EQ(level->price, remaining_order.price);
  ASSERT_EQ(level->head, &remaining_order);
  ASSERT_EQ(level->tail, &remaining_order);
  ASSERT_EQ(level->remaining_qty, remaining_order.remaining_qty);
  ASSERT_FALSE(book.is_resting(removed_order, side));
  ASSERT_TRUE(book.is_resting(remaining_order, side));
}

TEST_P(OrderBookReduceRestingSideTest, DoesNotModifyOppositeSide) {
  OrderBook<1> book;
  const Side side = GetParam();
  const Side opposite = opposite_side(side);
  Order reduced_order = create_order(0, 10, 10);
  Order opposite_order = create_order(1, 20, 20);

  ASSERT_TRUE(book.try_add_resting(reduced_order, side));
  ASSERT_TRUE(book.try_add_resting(opposite_order, opposite));
  ASSERT_TRUE(book.reduce_resting(reduced_order, side, reduced_order.remaining_qty));

  ASSERT_THAT(best_level(book, side), ::testing::IsNull());
  const auto* opposite_level = best_level(book, opposite);
  ASSERT_THAT(opposite_level, ::testing::NotNull());
  ASSERT_EQ(opposite_level->price, opposite_order.price);
  ASSERT_EQ(opposite_level->head, &opposite_order);
  ASSERT_EQ(opposite_level->tail, &opposite_order);
  ASSERT_EQ(opposite_level->remaining_qty, opposite_order.remaining_qty);
  ASSERT_FALSE(book.is_resting(reduced_order, side));
  ASSERT_TRUE(book.is_resting(opposite_order, opposite));
}

INSTANTIATE_TEST_SUITE_P(BothSides, OrderBookReduceRestingSideTest,
                         ::testing::Values(Side::Buy, Side::Sell), side_name);
INSTANTIATE_TEST_SUITE_P(BothSides, OrderBookReduceRestingThreeOrdersSideTest,
                         ::testing::Values(Side::Buy, Side::Sell), side_name);

} // namespace
} // namespace test