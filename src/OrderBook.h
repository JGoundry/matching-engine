#pragma once

#include "Order.h"

#include <vector>

// Price-time priority used

struct PriceLevel final {
  Price price;
  Quantity qty;
  Order *head;
  Order *tail;
};

class OrderBook final {
private:
  std::vector<PriceLevel> asks_;
  std::vector<PriceLevel> bids_;

public:
  constexpr explicit OrderBook() {
    asks_.reserve(10'000);
    bids_.reserve(10'000);
  }

  OrderBook(const OrderBook &rhs) = delete;
  OrderBook &operator=(const OrderBook &rhs) = delete;
  OrderBook(OrderBook &&rhs) = delete;
  OrderBook &operator=(OrderBook &&rhs) = delete;

  [[nodiscard]] constexpr Price best_bid() const {
    const std::size_t N = bids_.size();
    return N ? bids_[N - 1].price : 0;
  }
  [[nodiscard]] constexpr Price best_ask() const {
    const std::size_t N = asks_.size();
    return N ? asks_[N - 1].price : 0;
  }
  [[nodiscard]] constexpr Price mid_price() const {
    return best_ask() + best_bid() / 2;
  }
  [[nodiscard]] constexpr bool is_spread_crossed(Price price, Side side) const {
    return side == Side::Sell ? best_bid() >= price : best_ask() <= price;
  }
  [[nodiscard]] constexpr bool is_healthy() const {
    return !bids_.empty() && !asks_.empty() && best_bid() < best_ask();
  }
};
