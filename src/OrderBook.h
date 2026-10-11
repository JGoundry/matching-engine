#pragma once

#include "Order.h"

#include <type_traits>
#include <vector>

// Price-time priority used

/**
 * @brief Holds limit orders in time priority (FIFO) for a specific price level
 *        of the OrderBook.
 *
 * Aligned to 32 byte for exactly 2 objects on standard 64 bit dcache line.
 *
 * @see OrderBook, Order
 */
struct alignas(32) PriceLevel final {
  Price price;
  Quantity qty;
  Order *head;
  Order *tail;
};
static_assert(sizeof(PriceLevel) == 32);
static_assert(alignof(PriceLevel) == 32);

/**
 * @brief
 */
class OrderBook final {
private:
  // invariant: asks ordered by operator>
  //            bids ordered by operator<
  std::vector<PriceLevel> asks_;
  std::vector<PriceLevel> bids_;

public:
  constexpr explicit OrderBook() {
    // - reserve memory on startup to avoid dynamic allocations at runtime
    // - 10'000 chosen arbitrarily - need context of real order flow
    // - may want to enforce this as maximum number of price levels
    asks_.reserve(10'000);
    bids_.reserve(10'000);
  }

  OrderBook(const OrderBook &rhs) = delete;
  OrderBook &operator=(const OrderBook &rhs) = delete;
  OrderBook(OrderBook &&rhs) = delete;
  OrderBook &operator=(OrderBook &&rhs) = delete;

  void add_resting(Order &order, Side side);
  void remove_resting(Order &order, Side side);
  void reduce_resting(Order &order, Side side, Quantity qty);

  [[nodiscard]] constexpr const PriceLevel *best_bid_level() const {
    return bids_.empty() ? nullptr : &bids_.back();
  }
  [[nodiscard]] constexpr const PriceLevel *best_ask_level() const {
    return asks_.empty() ? nullptr : &asks_.back();
  }
};
static_assert(!std::is_copy_constructible_v<OrderBook>,
              "OrderBook must not be copied.");
static_assert(!std::is_copy_assignable_v<OrderBook>,
              "OrderBook must not be copied.");
static_assert(!std::is_move_assignable_v<OrderBook>,
              "OrderBook must not be moved.");
static_assert(!std::is_move_constructible_v<OrderBook>,
              "OrderBook must not be moved.");
