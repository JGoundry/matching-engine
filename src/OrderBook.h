#pragma once

#include "Order.h"

#include <type_traits>
#include <vector>

// Price-time priority used

struct alignas(32) PriceLevel final {
  Price price;
  Quantity qty;
  Order *head;
  Order *tail;
};
static_assert(sizeof(PriceLevel) == 32);
static_assert(alignof(PriceLevel) == 32);

class OrderBook final {
private:
  // invariant: asks ordered by operator<
  //            bids ordered by operator>
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

  bool add_order(Order *order);
  bool remove_order(Order *order);
  bool modify_order(Order *order, Quantity qty);

  [[nodiscard]] constexpr Price best_bid() const {
    const std::size_t N = bids_.size();
    return N ? bids_[N - 1].price : 0;
  }
  [[nodiscard]] constexpr Price best_ask() const {
    const std::size_t N = asks_.size();
    return N ? asks_[N - 1].price : 0;
  }
  [[nodiscard]] constexpr Price mid_price() const {
    const Price ask = best_ask();
    const Price bid = best_bid();
    return bid + (ask - bid) / 2;
  }
  [[nodiscard]] constexpr bool is_spread_crossed(Price price, Side side) const {
    return side == Side::Sell ? best_bid() >= price : best_ask() <= price;
  }
  [[nodiscard]] constexpr bool is_healthy() const {
    return !bids_.empty() && !asks_.empty() && best_bid() < best_ask();
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
