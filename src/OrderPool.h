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
  Order *free_head_{};
  std::size_t available_{N};

public:
  constexpr explicit OrderPool() {
    for (size_t i{}; i < N; ++i) {
      orders_[i].next = free_head_;
      free_head_ = &orders_[i];
    }
  }

  [[nodiscard]] constexpr std::size_t capacity() const noexcept { return N; }
  [[nodiscard]] constexpr std::size_t available() const noexcept {
    return available_;
  }

  [[nodiscard]] constexpr Order *try_allocate() noexcept {
    if (!free_head_)
      return nullptr;
    --available_;
    Order *free_order = free_head_;
    free_head_ = free_order->next;
    free_order->next = nullptr;
    return free_order;
  }

  // precondition: order was allocated by this pool and removed from book
  constexpr void deallocate(Order &order) noexcept {
    ++available_;
    order = Order{};
    order.next = free_head_;
    free_head_ = &order;
  }

  OrderPool(const OrderPool &rhs) = delete;
  OrderPool &operator=(const OrderPool &rhs) = delete;
  OrderPool(OrderPool &&rhs) = delete;
  OrderPool &operator=(OrderPool &&rhs) = delete;
};
