#pragma once

/**
 * @brief Response returned from `MatchingEngine` on receiving `InboundCommand`.
 */
enum class OutboundResponse : std::uint8_t { Accepted, Rejected };
