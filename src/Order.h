#pragma once

#include "Types.h"

#include <type_traits>

/**
 * @brief A unique order in MatchingEngine order properties and intrusive
 *        singly linked list.
 *
 * Aligned to 32 byte for exactly 2 objects on standard 64 bit dcache line.
 *
 * @see MatchingEngine OrderPool, OrderBook, PriceLevel
 */
struct alignas(32) Order final {
  OrderId id;
  Quantity qty;
  Price price;
  Order *next;
  // Doesnt include side or type currently
  // Debating on wether to add or... let it be implied
  // by the book and matching engine to save bytes
};
static_assert(sizeof(Order) == 32);
static_assert(alignof(Order) == 32);
static_assert(std::is_trivially_copyable_v<Order>);
static_assert(std::is_trivially_destructible_v<Order>);
