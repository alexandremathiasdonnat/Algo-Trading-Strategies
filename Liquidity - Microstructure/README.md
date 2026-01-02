# Liquidity / Order-Flow / Microstructure - Strategy Pack (Order Flow Imbalance)

## 1) Strategy Family: Liquidity / Order-Flow / Microstructure

### Core idea
*The alpha hypothesis is that price movements are driven by liquidity and order-flow imbalances, not by price patterns alone.*
*At short horizons, price changes are the mechanical result of aggressive orders consuming available liquidity at the best bid and ask.*

Microstructure strategies aim to exploit:
- temporary liquidity imbalances,
- order-flow pressure,
- asymmetric reactions to aggressive buying or selling.

### Main strategy archetypes in this family
- **Liquidity grab / stop runs**  
  Exploit zones where resting liquidity is likely to be consumed.
- **Stop hunt fade / continuation**  
  Trade the reaction after liquidity has been cleared.
- **VWAP anchoring**  
  Model interactions between price and institutional reference levels.
- **Order-flow imbalance**  
  Trade directional pressure induced by bid/ask volume imbalance.

This folder implements one baseline representative strategy:
**Order-Flow Imbalance Alpha**.

## 2) Structure and scope

This folder intentionally differs from candle-based strategy packs.

Microstructure logic:
- does not rely on OHLC candles,
- does not rely on broker execution models,
- operates at the level of order arrivals and liquidity queues.

Therefore, the implementation is split into two autonomous C++ components:
- a **limit order book (LOB) simulator**,
- an **order-flow-based trading signal engine**.

There is:
- no MT5 component,
- no external market data,
- no CSV,
- no networking.

The objective is to demonstrate:
- correct modeling of order flow and liquidity,
- correct handling of state transitions in a LOB,
- correct extraction of alpha from imbalance dynamics.

## 3) Limit Order Book (LOB) simulator

The LOB simulator models a simplified electronic market with:
- best bid and best ask prices,
- queue sizes on each side,
- stochastic arrivals of market and limit orders,
- cancellations.

Order arrivals follow Poisson-like processes with configurable intensities.

The simulator outputs a stream of market states:
- bid price, ask price,
- bid queue size, ask queue size,
- executed trades.

This creates a controlled environment to test microstructure hypotheses.

## 4) Order-flow imbalance alpha

The alpha signal is based on **queue imbalance** at the top of the book.

Let:
- `Q_bid(t)` be the size of the best bid queue,
- `Q_ask(t)` be the size of the best ask queue.

Define the imbalance: I(t) = (Q_bid(t) − Q_ask(t)) / (Q_bid(t) + Q_ask(t))


Interpretation:
- `I(t) → +1` : strong buy-side pressure,
- `I(t) → −1` : strong sell-side pressure.

### Trading logic

- go **long** when `I(t) > θ`,
- go **short** when `I(t) < −θ`,
- stay flat otherwise.

The signal is purely mechanical and evaluated on the current LOB state.

## 5) Implemented strategy: Order-Flow Imbalance

### State variables

`position_state ∈ {FLAT, LONG, SHORT}`  
Current exposure. At most one position at a time.

`Q_bid(t), Q_ask(t)`  
Queue sizes at the best bid and ask.

`I(t)`  
Normalized order-flow imbalance.

### Entry / exit rules

- **Entry**:
  - long if `I(t) > θ`,
  - short if `I(t) < −θ`.

- **Exit**:
  - exit to flat when `|I(t)| < θ_exit`,
  - or after a maximum holding time.

Risk management is intentionally minimal:
- fixed holding horizon,
- no price-based SL/TP (focus is on microstructure signal correctness).

## 6) Files
- `lob_simulator.cpp`: simplified limit order book simulator
- `order_flow_alpha.cpp`: imbalance-based trading logic and evaluation

## 7) General Disclaimer 

A real consistent algorithmic trading strategy =

**Signal** (clear alpha source)  
\+ **Regime awareness** (when the signal applies)  
\+ **Timing** (reaction speed)  
\+ **Risk management** (inventory control, exposure limits)

Many retail traders misuse microstructure concepts because they:
- apply indicators to candles instead of order flow,
- ignore liquidity mechanics,
- confuse price patterns with execution effects.

This repository focuses on **mechanical correctness and conceptual clarity**.
The goal is not to replicate real exchange microstructure, but to demonstrate how liquidity and order-flow imbalance can generate exploitable signals in a controlled setting. Performance claims and real-market execution are intentionally outside the scope of this repository.

---

***Alexandre Mathias DONNAT, Sr***