#pragma once

#include "Order.h"

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
    Order *head;
    Order *tail;
  };
  static_assert(sizeof(PriceLevel) == 32);
  static_assert(alignof(PriceLevel) == 32);

private:
  std::vector<PriceLevel> asks_; //< sorted in descending order; `asks_.back()` is best ask
  std::vector<PriceLevel> bids_; //< sorted in ascending order; `bids_.back()` is best bid

public:
  constexpr explicit OrderBook();

  OrderBook(const OrderBook &) = delete;
  OrderBook &operator=(const OrderBook &) = delete;
  OrderBook(OrderBook &&) = delete;
  OrderBook &operator=(OrderBook &&) = delete;

  /// @brief Attempts to append an order to its side's FIFO price level queue.
  /// @pre `order.remaining_qty > 0`.
  /// @pre `order` is not resting in this book already.
  /// @returns true if `order` was linked into the book; false only if its
  /// price has no existing level on `side` and that side has reached capacity.
  [[nodiscard]] bool try_add_resting(Order &order, Side side);

  /// @brief Reduces an order's remaining quantity.
  /// @pre `reduction_qty > 0 && reduction_qty <= order.remaining_qty`.
  /// @pre `order` is currently linked into this book on `side`.
  /// @returns true if the reduction fully filled and unlinked `order`; false
  /// if `order` remains resting with positive remaining quantity.
  /// @note Removes the containing price level if `order` was its final member.
  [[nodiscard]] bool reduce_resting(Order &order, Side side, Quantity reduction_qty);

  /// @brief Unlinks `order` from specified side of book.
  /// @pre `order` is currently linked into this book on `side`.
  /// @note Removes the containing price level if it becomes empty. Does not
  /// deallocate `order`.
  void remove_resting(Order &order, Side side);

  [[nodiscard]] bool is_resting(const Order &order, Side side) const noexcept;

  /// @returns The best bid level, or nullptr if no bids exist.
  /// @note The returned pointer is valid until the next non-const operation.
  [[nodiscard]] constexpr const PriceLevel *best_bid_level() const noexcept;

  /// @returns The best ask level, or nullptr if no asks exist.
  /// @note The returned pointer is valid until the next non-const operation.
  [[nodiscard]] constexpr const PriceLevel *best_ask_level() const noexcept;

  /// @returns The highest priority order at the best opposite side level, or nullptr if
  /// the opposite side is empty.
  [[nodiscard]] Order* best_opposite_order(Side aggressor_side) const noexcept;

  /// @returns true if `limit_price` crosses the best opposite side price.
  [[nodiscard]] constexpr bool is_spread_crossed(
      Price limit_price, Side aggressor_side) const noexcept;

  /// @brief Checks whether an immediately executable quantity exists.
  /// @pre `required_qty > 0`.
  /// @returns true if `required_qty` can execute at prices no worse than
  /// `limit_price`.
  [[nodiscard]] bool can_fully_execute(Price limit_price,
                                        Side aggressor_side,
                                        Quantity required_qty) const noexcept;
};

template <size_t N>
constexpr OrderBook<N>::OrderBook() {
  asks_.reserve(N);
  bids_.reserve(N);
}

template <size_t N>
constexpr const typename OrderBook<N>::PriceLevel *
OrderBook<N>::best_bid_level() const noexcept {
  return bids_.empty() ? nullptr : &bids_.back();
}

template <size_t N>
constexpr const typename OrderBook<N>::PriceLevel *
OrderBook<N>::best_ask_level() const noexcept {
  return asks_.empty() ? nullptr : &asks_.back();
}

static_assert(!std::is_copy_constructible_v<OrderBook<1>>,
              "OrderBook must not be copied.");
static_assert(!std::is_copy_assignable_v<OrderBook<1>>,
              "OrderBook must not be copied.");
static_assert(!std::is_move_assignable_v<OrderBook<1>>,
              "OrderBook must not be moved.");
static_assert(!std::is_move_constructible_v<OrderBook<1>>,
              "OrderBook must not be moved.");