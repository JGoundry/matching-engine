#pragma once

#include "Order.h"

#include <cstddef>
#include <type_traits>
#include <vector>

/**
 * @brief Holds limit orders in time priority (FIFO) for a specific price level
 * of the `OrderBook`.
 *
 * @par Alignment
 * Aligned to 32 byte for exactly 2 objects on standard 64 bit dcache line.
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
 * @brief Limit Order Book using price-time priority.
 * @tparam N Number of PriceLevel allocated per orderbook side
 * @note Construction preallocates storage for `N` price levels per side.
 * Implementations of mutating operations reject a new price level once
 * that side reaches `N`; neither vector shall be grown.
 */
template <size_t N> class OrderBook final {
private:
  // invariant: asks ordered by operator>
  //            bids ordered by operator<
  std::vector<PriceLevel> asks_;
  std::vector<PriceLevel> bids_;

public:
  constexpr explicit OrderBook() {
    asks_.reserve(N);
    bids_.reserve(N);
  }

  OrderBook(const OrderBook &) = delete;
  OrderBook &operator=(const OrderBook &) = delete;
  OrderBook(OrderBook &&) = delete;
  OrderBook &operator=(OrderBook &&) = delete;

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
static_assert(!std::is_copy_constructible_v<OrderBook<1>>,
              "OrderBook must not be copied.");
static_assert(!std::is_copy_assignable_v<OrderBook<1>>,
              "OrderBook must not be copied.");
static_assert(!std::is_move_assignable_v<OrderBook<1>>,
              "OrderBook must not be moved.");
static_assert(!std::is_move_constructible_v<OrderBook<1>>,
              "OrderBook must not be moved.");
