# Matching Engine

> **Work in progress:** `OrderBook` and `OrderPool` are implemented. Matching,
> order lifecycle handling, command validation, and outbound event emission are not.

## Overview

This is a C++23 limit order book and matching-engine project. `OrderBook`
stores bids and asks in sorted price levels, with orders queued FIFO at each
level. The intended engine processes one book's commands serially on a single
worker.

## Architecture

```text
InboundCommand -> MatchingEngine -> OrderBook
                              |-> OrderPool
                              |-> Order-ID index (provisional)
                              |-> OutboundResponse (provisional)
                              `-> Order and execution events (TODO)
```

Each `MatchingEngine` owns one book, one preallocatd order pool, and an
order ID index for lookup. The book links orders but does not manage their
lifetime, that responsibility is on the matching engine. A shared lock-free
queue may be added later for command ingress.

## Design choices

Price levels live in sorted vectors: bids ascend, asks descend, and the best
level is at the back on both sides. Best-level lookup is O(1); binary search
finds an existing level in O(log L), where L is the number of levels. Adding a
new level shifts entries and is O(L).

Orders at a level form an intrusive singly linked FIFO queue. Therefore,
cancelling an order other than the head takes O(Q) to find its predecessor,
where Q is the queue length. This choice was made to keep `Order` at 32-bytes;
this decision may change later after benchmarking.

`Order` and `PriceLevel` are 32-byte aligned. The layout is intended to fit two
objects on a standard 64-byte cache line; that is an initial design choice,
not a measured performance result.

20,000 orders and 10,000 price levels per side were used as placeholders; they
aren't based on measured order flow. The pool preallocates its orders at
construction and reuses freed orders LIFO; the book reserves space for its
levels and refuses a new, distinct level when that side is full. 

Prices and quantities are unsigned 64-bit integers. Prices are in ticks, not
floating point. The engine still needs command-boundary validation and defined
numeric bounds, including protection against quantity overflow.

The skeleton uses `std::unordered_map` for the order ID index and reserves its
buckets at construction; inserting entries may still allocate map nodes. This
is temporary. A replacement will be chosen after comparing options against the
expected workload.

## Current TODO

- Design and implement `MatchingEngine`: limit/market matching, GTC/IOC/FOK
  handling, and order lifecycle.
- Define the outbound event contract and emit order and execution events.
- Validate commands at ingress (current internal preconditions use `assert`,
  which is disabled in release builds); handle duplicate IDs and numeric bounds.
- Keep the book, index, and pool consistent through cancel, fill, and rejection.
- Pool exhaustion currently returns `Rejected`; define its full response/event
  behavior and handle the book's price-level capacity limit.
- Benchmark the data structures and the alignment choices under representative
  order and cancellation workloads.
- Consider a shared lock-free queue for command ingress. Regardless of how
  commands are submitted, one worker must process each book's mutations
  serially; define how concurrent submissions are ordered.