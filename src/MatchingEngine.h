#pragma once

#include "InboundCommand.h"
#include "Order.h"
#include "OrderBook.h"
#include "OrderPool.h"
#include "OutboundResponse.h"

#include <type_traits>
#include <unordered_map>

class MatchingEngine final {
private:
  using OrderIndex = std::unordered_map<OrderId, Order *>;
  OrderBook<10'000> book_;
  OrderPool<20'000> pool_;
  OrderIndex index_;

public:
  MatchingEngine(const MatchingEngine &rhs) = delete;
  MatchingEngine &operator=(const MatchingEngine &rhs) = delete;
  MatchingEngine(MatchingEngine &&rhs) = delete;
  MatchingEngine &operator=(MatchingEngine &&rhs) = delete;

  // TODO Upgrade ingestion to read from a shared lock free queue instead of
  //      using this function
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
