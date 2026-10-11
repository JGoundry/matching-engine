#pragma once

#include "Order.h"

#include <cstddef>
#include <vector>

// Fixed capacity pool of preallocated Order objects.
//
// Thread safety:
// This type is not thread-safe and provides no thread safety garuntees.
//
// Lifetime:
// Pointers returned by try_allocate() remain valid until deallocate() is
// called or the pool is destroyed. The pool must outlive every checked-out
// order.
//
// Ownership:
// An allocated Order is exclusively owned by the caller until it is returned.
// The caller must remove it from every OrderBook and index before deallocate().

template <std::size_t N> class OrderPool final {
private:
  std::vector<Order> orders_{N, Order{}};
  Order *head_{}; // free list
  std::size_t available_{N};

public:
  constexpr explicit OrderPool() {
    // link orders into free list starting at head_
    for (size_t i{}; i < N; ++i) {
      orders_[i].next = head_;
      head_ = &orders_[i];
    }
  }

  OrderPool(const OrderPool &rhs) = delete;
  OrderPool &operator=(const OrderPool &rhs) = delete;
  OrderPool(OrderPool &&rhs) = delete;
  OrderPool &operator=(OrderPool &&rhs) = delete;

  [[nodiscard]] constexpr std::size_t capacity() const noexcept { return N; }
  [[nodiscard]] constexpr std::size_t available() const noexcept {
    return available_;
  }

  [[nodiscard]] constexpr Order *try_allocate() noexcept {
    if (!head_)
      return nullptr;
    --available_;
    Order *order = head_;
    head_ = order->next;
    order->next = nullptr;
    return order;
  }

  // precondition: order was allocated by this pool and removed from book
  constexpr void deallocate(Order &order) noexcept {
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
