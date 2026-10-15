#pragma once

/**
 * @file Types.h
 * @brief Fundamental aliases and type definitions.
 */

#include <cstdint>

/// @brief Unique identifier representing an `Order`.
using OrderId = uint64_t;

/// @brief `Quantity` of asset.
using Quantity = uint64_t;

/// @brief `Price` specified in ticks.
/// @note Unsigned integer excludes negative prices.
using Price = uint64_t;

enum class Side : uint8_t { None, Buy, Sell };
enum class CommandType : uint8_t { New, Cancel };
enum class OrderType : uint8_t { None, LimitOrder, MarketOrder };
enum class TimeInForce : uint8_t { None, GoodTilCancelled, ImmediateOrCancel, FillOrKill };

static_assert(sizeof(Side) == 1);
static_assert(sizeof(CommandType) == 1);
static_assert(sizeof(OrderType) == 1);
static_assert(sizeof(TimeInForce) == 1);
