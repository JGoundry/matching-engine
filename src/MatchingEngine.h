#pragma once

#include "Order.h"
#include "OrderBook.h"
#include "OrderPool.h"

#include <unordered_map>

class MatchingEngine final {
private:
  using OrderIndex = std::unordered_map<OrderId, Order *>;
  OrderPool<10'000> pool_;
  OrderBook book_;
  OrderIndex index_;

public:
  MatchingEngine(const MatchingEngine &rhs) = delete;
  MatchingEngine &operator=(const MatchingEngine &rhs) = delete;
  MatchingEngine(MatchingEngine &&rhs) = delete;
  MatchingEngine &operator=(MatchingEngine &&rhs) = delete;
  OutboundResponse process(const InboundCommand &cmd) { return {}; }
};
