#pragma once

#include "InboundCommand.h"
#include "Order.h"
#include "OrderBook.h"
#include "OrderPool.h"
#include "OutboundResponse.h"

#include <type_traits>
#include <unordered_map>

/**
 * @brief Single-writer matching shard with stable identity for its lifetime.
 *
 * Construct and preallocate this type before routing commands to it. Calls to
 * `process()` must be serialized by its owning shard or worker thread.
 */
class MatchingEngine final {
private:
  using OrderIndex = std::unordered_map<OrderId, Order&>;
  OrderBook<10'000> book_;
  OrderPool<20'000> pool_;
  OrderIndex index_;

public:
  MatchingEngine() {
    index_.reserve(pool_.capacity());
  }

  MatchingEngine(const MatchingEngine &) = delete;
  MatchingEngine &operator=(const MatchingEngine &) = delete;
  MatchingEngine(MatchingEngine &&) = delete;
  MatchingEngine &operator=(MatchingEngine &&) = delete;

  /// @todo Upgrade InboundCommand ingestion to read from a shared lock free queue.
  [[nodiscard]] OutboundResponse process(const InboundCommand &cmd);

private:
  [[nodiscard]] OutboundResponse cancel_order(OrderId id);
  [[nodiscard]] OutboundResponse limit_order(const Order &order, Side side);
  [[nodiscard]] OutboundResponse market_order(const Order &order, Side side);
};
static_assert(!std::is_copy_constructible_v<MatchingEngine>,
              "MatchingEngine must not be copied.");
static_assert(!std::is_copy_assignable_v<MatchingEngine>,
              "MatchingEngine must not be copied.");
static_assert(!std::is_move_assignable_v<MatchingEngine>,
              "MatchingEngine must not be moved.");
static_assert(!std::is_move_constructible_v<MatchingEngine>,
              "MatchingEngine must not be moved.");
