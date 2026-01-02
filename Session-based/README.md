# Session-Based / Time-of-Day - Strategy Pack (London Breakout)

## 1) Strategy Family: Session-Based / Time-of-Day Strategies

### Core idea
*The alpha hypothesis is time-of-day asymmetry: liquidity, volume, and participant behavior are not stationary across the trading day.
Some sessions tend to be range-bound (compression), while others tend to exhibit volatility expansion and directional moves.*

### Main strategy archetypes in this family
- **London Breakout**  
  Trade the breakout of the Asian session range at the London open.
- **New York Open Range Breakout**  
  Exploit volatility expansion around the US market open.
- **Asia Range Fade**  
  Fade the boundaries of the Asian range assuming mean-reverting microstructure in low-liquidity regimes.
- **Intraday Time Filters**  
  Only trade during specific time windows where expected edge is higher.

This folder implements one baseline representative strategy: **London Breakout**.

## 2) Two independent implementations

This repository is designed as a example in algorithmic trading, showcasing how the same trading idea can be implemented coherently in two different programming environments, each serving a distinct purpose.

The two implementations share the same algorithmic logic, but are intentionally kept fully independent:

- no bridge between MQL5 and C++
- no shared datasets or execution pipeline
- no attempt to simulate a production architecture

The objective is not redundancy, but conceptual clarity: demonstrating that the strategy can be expressed, reasoned about, and executed consistently across languages.

**What this is:**
- A demonstration of clean abstraction of a trading strategy
- A way to reason explicitly about signals, state transitions, and risk handling
- A controlled setting to compare how the same logic maps to different tools

**What this is NOT:**
- Not a production trading system
- Not a unified research or execution framework
- Not a claim of profitability or any form of investment advice

## 3) MQL5 / .mq5 implementation

MetaTrader 5 provides a practical and realistic environment for algorithmic trading experimentation.

It offers:
- direct access to historical and live market data via broker connections (demo accounts are easy to obtain),
- integrated IDE (.mq5 :  .cpp-like)
- an integrated strategy tester for backtesting and parameter exploration,
- a native execution model handling orders, positions, spreads, and symbol specifications,
- a fast feedback loop: code → compile → backtest → iterate.

In this project, the .mq5 implementation serves as the most direct way to test the strategy logic under real market conditions, within the constraints and assumptions of the MT5 ecosystem.

## 4) Standalone C++ implementation

The C++ implementation has a different objective.

Rather than replacing MetaTrader or acting as a live trading system, it forces a re-implementation of the strategy from first principles, outside of any broker-provided environment:

- explicit time-window logic (session boundaries),
- explicit range computation (Asian high/low),
- explicit stop-order logic (breakout entries),
- explicit position state management and risk exits (SL/TP),
- explicit trade tracking and performance aggregation.

Because this is a standalone program with no dependency on broker infrastructure, the strategy is executed on an internally generated synthetic intraday price series with a time-of-day volatility pattern (lower volatility in “Asia”, higher volatility around “London”).

This choice is deliberate and meaningful:
- it keeps the code fully autonomous and self-contained,
- it avoids hiding logic behind platform-specific abstractions,
- it highlights the mechanics of a session-based strategy (time windows, range, stop orders),
- it mirrors how quantitative research often starts with simplified or simulated environments before scaling to real datasets.

This C++ version therefore acts as a research-oriented counterpart to the MT5 implementation, and as a clean foundation for future extensions (e.g. real-data ingestion in a separate project, vectorized backtests, or multi-asset session studies).

## 5) The dual implemented strategy: London Breakout

### Signal definition

Let:
- `AsiaHigh(d)` be the highest price observed during the Asian session for day `d`
- `AsiaLow(d)` be the lowest price observed during the Asian session for day `d`
- `LondonOpen(d)` be the start time of the London session for day `d`
- `LondonClose(d)` be the end time of the London session for day `d`

The strategy assumes that Asia often forms a compression range, and that London open can trigger a volatility expansion that breaks this range.

### Trading rules (identical logic in both implementations)

The strategy is implemented as a finite-state trading system, with explicit signal evaluation and position state management.

#### Core state variables

`position_state ∈ {FLAT, LONG, SHORT}`  
Current exposure of the strategy. At most one position can be active at any time.

`AsiaHigh(d), AsiaLow(d)`  
Daily Asian range boundaries computed from historical bars strictly prior to the London session.

`buy_stop_level(d), sell_stop_level(d)`  
Breakout entry levels derived from the Asian range (optionally with a small buffer).

`SL`, `TP` (optional)  
Fixed stop-loss and take-profit levels defined by the strategy at entry.  
The underlying risk logic is identical in both implementations, while the parameterization unit differs:
- in MQL5, `SL`/`TP` are expressed in points (platform-native convention) and attached directly to the order,
- in the standalone C++ program, `SL`/`TP` are expressed as percentages of the entry price and evaluated in code.

#### Signal definition (session logic)

On each trading day `d`, at `LondonOpen(d)`:
1. compute `AsiaHigh(d)` and `AsiaLow(d)` from the Asian session window,
2. place two pending breakout orders:
   - a buy-stop at `buy_stop_level(d) = AsiaHigh(d) + buffer`,
   - a sell-stop at `sell_stop_level(d) = AsiaLow(d)  - buffer`,
3. both orders automatically expire at `LondonClose(d)` if not triggered.

This logic ensures the range is computed strictly before trading it (no lookahead).

#### State transition logic

**At London open (once per day):**
- if `position_state == FLAT` → place both pending orders (buy-stop and sell-stop)

**When a buy-stop is triggered:**
- if `position_state == FLAT` → open a long position and cancel the sell-stop

**When a sell-stop is triggered:**
- if `position_state == FLAT` → open a short position and cancel the buy-stop

At no point can the system hold more than one active position.

#### Risk management

When enabled, each position is initialized with:
- a fixed Stop-Loss (`SL`),
- a fixed Take-Profit (`TP`).

In the standalone C++ implementation, `SL` and `TP` are not stored as persistent variables.  
They are deterministically recomputed at each time step from the entry price `P₀`, the position direction `s ∈ {+1, −1}`, and fixed percentage parameters `α_SL`, `α_TP`:

**Stop-Loss:**  
`SL = P₀ · (1 − s · α_SL)`

**Take-Profit:**  
`TP = P₀ · (1 + s · α_TP)`

where `s = +1` for a long position and `s = −1` for a short position.

Positions are closed immediately when the current price crosses either threshold.  
Risk evaluation is performed independently of session logic once in a position.

#### Implementation note

The strategy logic is strictly event-driven:
- the Asian range is computed once per day from closed bars,
- pending orders are placed once per day at session start,
- position state transitions are explicit and deterministic,
- no parameter optimization, regime filtering, or portfolio-level logic is applied at this stage.

This section describes the exact same logical structure implemented in both the MQL5 Expert Advisor and the standalone C++ program, despite differences in language syntax and execution environment.

## 6) Files
- `London_Breakout.mq5`: MT5 Expert Advisor (market data + strategy tester)
- `London_Breakout.cpp`: standalone C++ program (synthetic intraday data, same logic, full run)

## 7) General Disclaimer 

A real consistent algorithmic trading strategy =

**Signal** (clear alpha source from a strategy family)  
\+ **Regime filter** (when the signal should be active)  
\+ **Timing** (how and when to enter / exit)  
\+ **Risk management** (position sizing, stops, drawdown control)

Many retail strategies fail because they:
- underestimate the impact of continuous macroeconomic news flows (especially in FX),
- mix indicators without a clear economic or statistical hypothesis,
- stack ad-hoc rules,
- endlessly tweak entries,
- without ever isolating where the alpha actually comes from.
- fail to properly account for key parameters such as transaction costs, spreads, and slippage.

This repository focuses on the design and implementation of trading strategies, rather than on ranking them by performance.
The work presented here operates at the level of mechanical / logical backtesting:
each strategy is tested in isolation to verify that it generates coherent signals, respects its trading rules, manages position states correctly (flat / long / short), and applies risk constraints (SL / TP) without logical errors, lookahead bias, or non-deterministic behavior.

Higher-level analyses, such as performance comparison, parameter optimization, and portfolio-level backtesting, require dedicated research frameworks, realistic cost modeling, and strict evaluation protocols, and are therefore intentionally outside the scope of this repository.

Here, I deliberately focus on isolating and implementing one clear alpha component at a time.

---

***Alexandre Mathias DONNAT, Sr***