#pragma once

#include <cstdint>
#include <type_traits>

enum class Side : uint8_t { Buy, Sell };
enum class CommandType : uint8_t { New, Cancel };
enum class OrderType : uint8_t { LimitOrder, MarketOrder };
enum class TimeInForce : uint8_t {
  GoodTilCancelled,
  ImmediateOrCancel,
  FillOrKill
};

using OrderId = uint64_t;
using Quantity = uint64_t;
using Price = uint64_t;

class alignas(32) InboundCommand final {
private:
  OrderId id_;
  CommandType commandType_;
  OrderType orderType_;
  Side side_;
  Price price_;
  Quantity qty_;

  constexpr explicit InboundCommand(OrderId id, CommandType commandType,
                                    OrderType orderType, Side side, Price price,
                                    Quantity qty)
      : id_(id), commandType_(commandType), orderType_(orderType), side_(side),
        price_(price), qty_(qty) {}

public:
  [[nodiscard]] static constexpr InboundCommand market(OrderId id, Quantity qty,
                                                       Side side);
  [[nodiscard]] static constexpr InboundCommand limit(OrderId id, Quantity qty,
                                                      Price price, Side side);
  [[nodiscard]] static constexpr InboundCommand cancel(OrderId id);
};
static_assert(sizeof(InboundCommand) == 32);
static_assert(alignof(InboundCommand) == 32);
static_assert(std::is_trivially_copyable_v<InboundCommand>);

struct OutboundResponse {};
static_assert(sizeof(OutboundResponse) == 1);
static_assert(alignof(OutboundResponse) == 1);
static_assert(std::is_trivially_copyable_v<OutboundResponse>);

struct alignas(32) Order final {
  OrderId id;
  Quantity qty;
  Price price;
  Order *next;
};
static_assert(sizeof(Order) == 32);
static_assert(alignof(Order) == 32);
static_assert(std::is_trivially_copyable_v<Order>);
