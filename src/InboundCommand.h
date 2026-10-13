#pragma once

#include "Types.h"

#include <type_traits>

class alignas(32) InboundCommand final {
private:
  OrderId id_;                // 8
  Price price_;               // 8
  Quantity qty_;              // 8
  CommandType commandType_;   // 1
  Side side_;                 // 1
  OrderType orderType_;       // 1
  TimeInForce time_in_force_; // 1

  constexpr explicit InboundCommand(OrderId id, CommandType commandType,
                                    OrderType orderType, Side side, Price price,
                                    Quantity qty) noexcept
      : id_(id), commandType_(commandType), orderType_(orderType), side_(side),
        price_(price), qty_(qty) {}

public:
  // Factory functions
  [[nodiscard]] static constexpr InboundCommand market(OrderId id, Quantity qty,
                                                       Side side) noexcept;
  [[nodiscard]] static constexpr InboundCommand
  limit(OrderId id, Quantity qty, Price price, Side side) noexcept;
  [[nodiscard]] static constexpr InboundCommand cancel(OrderId id) noexcept;

  // Inline getters
  [[nodiscard]] constexpr OrderId id() const noexcept { return id_; }
  [[nodiscard]] constexpr CommandType commandType() const noexcept {
    return commandType_;
  }
  [[nodiscard]] constexpr OrderType orderType() const noexcept {
    return orderType_;
  }
  [[nodiscard]] constexpr Side side() const noexcept { return side_; }
  [[nodiscard]] constexpr Price price() const noexcept { return price_; }
  [[nodiscard]] constexpr Quantity qty() const noexcept { return qty_; }
};
static_assert(sizeof(InboundCommand) == 32);
static_assert(alignof(InboundCommand) == 32);
static_assert(std::is_trivially_copyable_v<InboundCommand>);
