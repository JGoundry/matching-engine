#pragma once

#include "Types.h"

#include <type_traits>

class alignas(32) InboundCommand final {
private:
  OrderId id_;               // 8
  Price price_;              // 8
  Quantity qty_;             // 8
  CommandType command_type_; // 1
  OrderType order_type_;     // 1
  Side side_;                // 1
  TimeInForce tif_;          // 1

  constexpr explicit InboundCommand(OrderId id, Price price, Quantity qty, CommandType command_type,
                                    OrderType order_type, Side side, TimeInForce tif) noexcept
      : id_(id), price_(price), qty_(qty), command_type_(command_type), order_type_(order_type),
        side_(side), tif_(tif) {}

public:
  /// @brief Factory function for market order command
  [[nodiscard]] static constexpr InboundCommand market(OrderId id, Quantity qty,
                                                       Side side) noexcept {
    return InboundCommand(id, 0, qty, CommandType::New, OrderType::MarketOrder, side,
                          TimeInForce::None);
  }

  /// @brief Factory function for limit order command
  /// @pre `tif` must not be `None`
  [[nodiscard]] static constexpr InboundCommand limit(OrderId id, Price price, Quantity qty,
                                                      Side side, TimeInForce tif) noexcept {
    return InboundCommand(id, price, qty, CommandType::New, OrderType::LimitOrder, side, tif);
  }

  /// @brief Factory function for cancel order command
  [[nodiscard]] static constexpr InboundCommand cancel(OrderId id) noexcept {
    return InboundCommand(id, 0, 0, CommandType::Cancel, OrderType::None, Side::None,
                          TimeInForce::None);
  }

  /// @name Accessors
  /// @{
  [[nodiscard]] constexpr OrderId id() const noexcept { return id_; }
  [[nodiscard]] constexpr Price price() const noexcept { return price_; }
  [[nodiscard]] constexpr Quantity qty() const noexcept { return qty_; }
  [[nodiscard]] constexpr CommandType command_type() const noexcept { return command_type_; }
  [[nodiscard]] constexpr OrderType order_type() const noexcept { return order_type_; }
  [[nodiscard]] constexpr Side side() const noexcept { return side_; }
  [[nodiscard]] constexpr TimeInForce time_in_force() const noexcept { return tif_; }
  /// @}
};
static_assert(sizeof(InboundCommand) == 32);
static_assert(alignof(InboundCommand) == 32);
static_assert(std::is_standard_layout_v<InboundCommand>);
static_assert(std::is_trivially_copyable_v<InboundCommand>);