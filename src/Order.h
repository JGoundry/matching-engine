#pragma once

#include "Types.h"

#include <type_traits>

/**
 * @brief A collection of properties representing a single unique limit `Order`.
 * @note Stores an intrusive single linked list.
 *
 * @par Alignment
 * Aligned to 32 byte for exactly 2 objects on standard 64 bit dcache line.
 */
struct alignas(32) Order final {
  OrderId id;
  Price price;
  Quantity remaining_qty;
  Order* next;
  bool operator==(const Order&) const = default;
};
static_assert(sizeof(Order) == 32);
static_assert(alignof(Order) == 32);
static_assert(std::is_standard_layout_v<Order>);
static_assert(std::is_trivially_default_constructible_v<Order>);
static_assert(std::is_trivially_copyable_v<Order>);