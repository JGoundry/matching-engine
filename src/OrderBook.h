#pragma once

#include "Order.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <type_traits>
#include <vector>

/**
 * @brief Limit Order Book using price-time priority.
 * @tparam N Maximum number of active price levels per order book side.
 * @note Construction reserves storage for `N` price levels per side. A new
 * price level cannot be added once the relevant side reaches this capacity.
 * @note `OrderBook` does not manage `Order` lifetime, will not allocate
 * or deallocate an `Order`.
 */
template <size_t N> class OrderBook final {
public:
  /**
   * @brief Holds limit orders in time priority (FIFO) for a specific price
   * level of this order book.
   *
   * @par Alignment
   * Aligned to 32 bytes so two levels fit in a typical 64-byte cache line.
   */
  struct alignas(32) PriceLevel final {
    Price price;
    Quantity remaining_qty;
    Order* head;
    Order* tail;
  };
  static_assert(sizeof(PriceLevel) == 32);
  static_assert(alignof(PriceLevel) == 32);

private:
  std::vector<PriceLevel> asks_; //< sorted in descending order; `asks_.back()` is best ask
  std::vector<PriceLevel> bids_; //< sorted in ascending order; `bids_.back()` is best bid

public:
  constexpr explicit OrderBook();

  OrderBook(const OrderBook&) = delete;
  OrderBook& operator=(const OrderBook&) = delete;
  OrderBook(OrderBook&&) = delete;
  OrderBook& operator=(OrderBook&&) = delete;

  /// @brief Attempts to append an order to its side's FIFO price level queue.
  /// @pre `order.remaining_qty > 0`.
  /// @pre `order` is not resting in this book already.
  /// @pre `side` is not Side::None
  /// @returns true if `order` was linked into the book; false only if its
  /// price has no existing level on `side` and that side has reached capacity.
  [[nodiscard]] bool try_add_resting(Order& order, Side side);

  /// @brief Reduces an order's remaining quantity.
  /// @pre `side` is not Side::None
  /// @pre `reduction_qty > 0 && reduction_qty <= order.remaining_qty`.
  /// @pre `order` is currently linked into this book on `side`.
  /// @returns true if the reduction fully filled and unlinked `order`; false
  /// if `order` remains resting with positive remaining quantity.
  /// @note Removes the containing price level if `order` was its final member.
  [[nodiscard]] bool reduce_resting(Order& order, Side side, Quantity reduction_qty);

  /// @brief Unlinks `order` from specified side of book.
  /// @pre `order` is currently linked into this book on `side`.
  /// @note Removes the containing price level if it becomes empty. Does not
  /// deallocate `order`.
  void remove_resting(Order& order, Side side);

  /// @returns The best bid level, or nullptr if no bids exist.
  /// @note The returned pointer is valid until the next non-const operation.
  [[nodiscard]] constexpr const PriceLevel* best_bid_level() const noexcept;

  /// @returns The best ask level, or nullptr if no asks exist.
  /// @note The returned pointer is valid until the next non-const operation.
  [[nodiscard]] constexpr const PriceLevel* best_ask_level() const noexcept;

  /// @returns The highest priority order at the best opposite side level, or nullptr if
  /// the opposite side is empty.
  [[nodiscard]] Order* best_opposite_order(Side aggressor_side) const noexcept;

  /// @returns true if `limit_price` crosses the best opposite side price.
  [[nodiscard]] constexpr bool is_spread_crossed(Price limit_price,
                                                 Side aggressor_side) const noexcept;

  /// @brief Checks whether an immediately executable quantity exists.
  /// @pre `required_qty > 0`.
  /// @returns true if `required_qty` can execute at prices no worse than
  /// `limit_price`.
  [[nodiscard]] bool can_fully_execute(Price limit_price, Side aggressor_side,
                                       Quantity required_qty) const noexcept;

  /// @brief Checks if `order` rests in book.
  /// @pre `side` is not Side::None
  /// @returns true if order rests in book; false otherwise.
  [[nodiscard]] bool is_resting(const Order& order, Side side) const noexcept;

private:
  [[nodiscard]] bool is_resting_anywhere(const Order& order) const noexcept;
};

template <size_t N> constexpr OrderBook<N>::OrderBook() {
  asks_.reserve(N);
  bids_.reserve(N);
}

template <size_t N> bool OrderBook<N>::try_add_resting(Order& order, Side side) {
  assert(side != Side::None && "side must not be Side::None");
  assert(order.remaining_qty > 0 && "remaining_qty must be greater than zero");
  assert(!is_resting_anywhere(order) && "order must not be in book already");

  auto& levels = (side == Side::Buy ? bids_ : asks_);
  const auto it =
      (side == Side::Buy
           ? std::ranges::lower_bound(levels, order.price, std::less{}, &PriceLevel::price)
           : std::ranges::lower_bound(levels, order.price, std::greater{}, &PriceLevel::price));

  const bool level_exists = it != levels.end() && it->price == order.price;

  if (!level_exists && levels.size() == N)
    return false;

  order.next = nullptr;
  if (level_exists) {
    PriceLevel& level = *it;
    level.tail->next = &order;
    level.tail = &order;
    level.remaining_qty += order.remaining_qty;
  } else {
    levels.insert(it, PriceLevel{.price = order.price,
                                 .remaining_qty = order.remaining_qty,
                                 .head = &order,
                                 .tail = &order});
  }

  return true;
}

/// @returns true if the reduction fully filled and unlinked `order`; false
/// if `order` remains resting with positive remaining quantity.
/// @note Removes the containing price level if `order` was its final member.
template <size_t N>
bool OrderBook<N>::reduce_resting(Order& order, Side side, Quantity reduction_qty) {
  assert(side != Side::None && "side must not be Side::None");
  assert(is_resting(order, side) && "order to be reduced must be resting in given side");
  assert(reduction_qty > 0 && "reduction_qty must be greater than zero");
  assert(order.remaining_qty >= reduction_qty &&
         "order.remaining_qty must be greater than or equal to reduction amount");

  // Find level
  auto& levels = (side == Side::Buy ? bids_ : asks_);
  const auto level_it =
      (side == Side::Buy
           ? std::ranges::lower_bound(levels, order.price, std::less{}, &PriceLevel::price)
           : std::ranges::lower_bound(levels, order.price, std::greater{}, &PriceLevel::price));

  assert(level_it != levels.end() && level_it->price == order.price);

  order.remaining_qty -= reduction_qty;
  level_it->remaining_qty -= reduction_qty;

  if (order.remaining_qty != 0)
    return false; // Order reduced but not fully filled

  // Order is fully filled, find order and remove from list. Could be O(1) with a doubly linked
  // list, but, Order is already 32 bytes, is acceptable for now :/

  const bool is_head = level_it->head == &order;
  const bool is_tail = level_it->tail == &order;

  if (is_head && is_tail) {
    levels.erase(level_it); // Only order at price level, delete level
  } else if (is_head) {
    level_it->head = order.next; // Make next order new head
  } else {
    // Find order before ours
    Order* order_it = level_it->head;
    while (order_it->next != &order) {
      order_it = order_it->next;
    }
    assert(order_it->next == &order && "order should be found resting at this level");
    if (is_tail) { // Set new tail
      order_it->next = nullptr;
      level_it->tail = order_it;
    } else { // Unlink order
      order_it->next = order.next;
    }
  }

  return true;
}

template <size_t N>
constexpr const OrderBook<N>::PriceLevel* OrderBook<N>::best_bid_level() const noexcept {
  return bids_.empty() ? nullptr : &bids_.back();
}

template <size_t N>
constexpr const OrderBook<N>::PriceLevel* OrderBook<N>::best_ask_level() const noexcept {
  return asks_.empty() ? nullptr : &asks_.back();
}

template <size_t N> bool OrderBook<N>::is_resting(const Order& order, Side side) const noexcept {
  assert(side != Side::None && "side must not be Side::None");

  const auto& levels = side == Side::Buy ? bids_ : asks_;
  const auto it =
      (side == Side::Buy
           ? std::ranges::lower_bound(levels, order.price, std::less{}, &PriceLevel::price)
           : std::ranges::lower_bound(levels, order.price, std::greater{}, &PriceLevel::price));

  // Check a level at given order price exists
  const bool level_exists = it != levels.end() && it->price == order.price;
  if (!level_exists)
    return false;

  // Check if order exists at existing price level
  Order* list_it = it->head;
  while (list_it) {
    if (list_it == &order)
      return true; // found order
    list_it = list_it->next;
  }

  return false;
}

template <size_t N> bool OrderBook<N>::is_resting_anywhere(const Order& order) const noexcept {
  return is_resting(order, Side::Buy) || is_resting(order, Side::Sell);
}

static_assert(!std::is_copy_constructible_v<OrderBook<1>>, "OrderBook must not be copied.");
static_assert(!std::is_copy_assignable_v<OrderBook<1>>, "OrderBook must not be copied.");
static_assert(!std::is_move_assignable_v<OrderBook<1>>, "OrderBook must not be moved.");
static_assert(!std::is_move_constructible_v<OrderBook<1>>, "OrderBook must not be moved.");