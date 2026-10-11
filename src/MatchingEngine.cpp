#include "MatchingEngine.h"
#include "Order.h"

#include <cassert>

OutboundResponse MatchingEngine::process(const InboundCommand &cmd) {
  if (cmd.commandType() == CommandType::Cancel) {
    return cancel_order(cmd.id());
  }

  assert(cmd.commandType() == CommandType::New);

  // Populate and dispatch pre-allocated Order

  Order *order = pool_.try_allocate();
  if (order == nullptr) [[unlikely]] {
    return {}; // todo
  }

  order->id = cmd.id();
  order->qty = cmd.qty();

  if (cmd.orderType() == OrderType::LimitOrder) {
    order->price = cmd.price();
    return limit_order(*order, cmd.side());
  }

  assert(cmd.orderType() == OrderType::MarketOrder);

  return market_order(*order, cmd.side());
}

OutboundResponse MatchingEngine::cancelOrder(OrderId id) {
  // book_.cancelOrder(id);
  return {};
}

OutboundResponse MatchingEngine::limitOrder(const Order &order, Side side) {
  if (book_.is_spread_crossed(order.price, side)) { // marketable limit order
  }
  return {};
}

OutboundResponse MatchingEngine::marketOrder(const Order &order, Side side) {
  return {};
}
