#pragma once

#include "Order.h"

#include <cassert>
#include <bitset>
#include <cstddef>
#include <vector>

/**
 * @brief Fixed-capacity pool of preallocated `Order` objects.
 * @tparam N Number of `Order` objects in the pool.
 * @note `OrderPool` is neither copyable nor movable.
 *
 * @par Thread safety
 * This type is not thread-safe and provides no thread-safety guarantees.
 *
 * @par Lifetime
 * Pointers returned by `try_allocate()` remain valid until `deallocate()` is
 * called or the pool is destroyed. The pool must outlive every checked-out
 * order.
 *
 * @par Ownership
 * An allocated `Order` is exclusively owned by the caller until it is returned.
 * The caller must remove it from every `OrderBook` and index before
 * `deallocate()`.
 */
template <size_t N> class OrderPool final {
private:
  std::vector<Order> orders_{N, Order{}}; //< Preallocated orders
  Order *head_{}; //< Head of the free list
  size_t available_{N}; //< Number of available orders
#ifndef NDEBUG
  std::bitset<N> allocated_{}; //< Bitset showing values in `orders_` that have been allocated
  [[nodiscard]] size_t index_of(const Order &order) const
      noexcept {
    for (size_t i{}; i < N; ++i) {
      if (&orders_[i] == &order)
        return i;
    }
    assert(false && "order does not belong to this pool");
    return N;
  }
#endif

public:
  /// @brief Constructs a pool containing `N` available orders.
  constexpr explicit OrderPool() {
    for (size_t i{}; i < N; ++i) {
      orders_[i].next = head_;
      head_ = &orders_[i];
    }
  }

  OrderPool(const OrderPool &) = delete;
  OrderPool &operator=(const OrderPool &) = delete;
  OrderPool(OrderPool &&) = delete;
  OrderPool &operator=(OrderPool &&) = delete;

  /// @brief Returns the total number of orders in the pool.
  [[nodiscard]] constexpr size_t capacity() const noexcept { return N; }

  /// @brief Returns the number of orders currently available for allocation.
  [[nodiscard]] constexpr size_t available() const noexcept {
    return available_;
  }

  /// @brief Allocates an order from the pool if one is available.
  /// @return A pointer to the allocated order, or `nullptr` if the pool is
  /// exhausted.
  [[nodiscard]] constexpr Order *try_allocate() noexcept {
    if (!head_)
      return nullptr;
    --available_;
    Order *order = head_;
    head_ = order->next;
    order->next = nullptr;
#ifndef NDEBUG
    const size_t index = index_of(*order);
    assert(!allocated_[index] && "free-list order is already allocated");
    allocated_.set(index);
#endif
    return order;
  }

  /// @brief Returns an allocated order to the pool.
  /// @param order Order to return.
  /// @pre `order` was allocated by this pool and removed from every
  /// `OrderBook` and index.
  constexpr void deallocate(Order &order) noexcept {
#ifndef NDEBUG
    const size_t index = index_of(order);
    assert(allocated_.test(index) &&
           "order was not allocated or was already deallocated");
    allocated_.reset(index);
#endif
    ++available_;
    order = Order{};
    order.next = head_;
    head_ = &order;
  }
};
static_assert(!std::is_copy_constructible_v<OrderPool<1>>,
              "OrderPool must not be copied.");
static_assert(!std::is_copy_assignable_v<OrderPool<1>>,
              "OrderPool must not be copied.");
static_assert(!std::is_move_assignable_v<OrderPool<1>>,
              "OrderPool must not be moved.");
static_assert(!std::is_move_constructible_v<OrderPool<1>>,
              "OrderPool must not be moved.");
