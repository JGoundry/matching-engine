#include "MatchingEngine.h"
#include "Order.h"

#include <cassert>

OutboundResponse MatchingEngine::process(const InboundCommand &cmd) {
  if (cmd.command_type() == CommandType::Cancel) {
    return cancel_order(cmd.id());
  }

  assert(cmd.command_type() == CommandType::New);

  // Populate and dispatch pre-allocated Order

  Order *order = pool_.try_allocate();
  if (order == nullptr) [[unlikely]] {
    return {}; // todo
  }

  order->id = cmd.id();
  order->qty = cmd.qty();

  if (cmd.order_type() == OrderType::LimitOrder) {
    order->price = cmd.price();
    return limit_order(*order, cmd.side());
  }

  assert(cmd.order_type() == OrderType::MarketOrder);

  return market_order(*order, cmd.side());
}

OutboundResponse MatchingEngine::cancel_order(OrderId id) {
  // book_.cancel_order(id);
  return {};
}

OutboundResponse MatchingEngine::limit_order(const Order &order, Side side) {
  // if (book_.is_spread_crossed(order.price, side)) { // marketable limit order
  // }
  return {};
}

OutboundResponse MatchingEngine::market_order(const Order &order, Side side) {
  return {};
}
