# Volatility-Based - Strategy Pack (ATR Expansion Breakout)

## 1) Strategy Family: Volatility-Based Strategies

### Core idea
*The alpha hypothesis is volatility mispricing: the market tends to under- or over-estimate future volatility, and volatility regimes are not constant.
Volatility-based strategies attempt to exploit volatility expansion after compression, or adapt behavior depending on whether the market is in a high- or low-volatility regime.*

### Main strategy archetypes in this family
- **Volatility Breakout**  
  Trade volatility expansion following a low-volatility compression regime.
- **Straddle / Strangle**  
  Bet on a large move without taking a directional view (options-based).
- **Regime Switching**  
  Adapt position sizing, entry logic, or strategy selection based on volatility regime.
- **ATR Expansion Models**  
  Enter when realized volatility (proxied by ATR) exceeds a threshold.

This folder implements one baseline representative strategy: **ATR Expansion Breakout**.

## 2) Two independent implementations

This repository is designed as a proof-of-work in algorithmic trading, showcasing how the same trading idea can be implemented coherently in two different programming environments, each serving a distinct purpose.

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

- explicit volatility measurement (ATR),
- explicit regime detection (low-vol compression vs expansion),
- explicit breakout logic (directional trigger),
- explicit position state management and risk exits (SL/TP),
- explicit trade tracking and performance aggregation.

Because this is a standalone program with no dependency on broker infrastructure, the strategy is executed on an internally generated synthetic price series with time-varying volatility regimes.

This choice is deliberate and meaningful:
- it keeps the code fully autonomous and self-contained,
- it avoids hiding logic behind platform-specific abstractions,
- it highlights the core mechanics of volatility regime detection and breakout triggering,
- it mirrors how quantitative research often starts with simplified or simulated environments before scaling to real datasets.

This C++ version therefore acts as a research-oriented counterpart to the MT5 implementation, and as a clean foundation for future extensions (e.g. real-data ingestion in a separate project, vectorized backtests, regime labels, or portfolio-level vol overlays).

## 5) The dual implemented strategy: ATR Expansion Breakout

### Signal definition

Let:
- `ATR(t)` be the Average True Range computed over a rolling window of length `N`
- `ATR_slow(t)` be a slower reference ATR (longer window) used as a baseline
- `P(t)` be the close price of the last closed bar

A volatility expansion event is detected when realized volatility rises significantly relative to its baseline:
- **Expansion trigger** if `ATR(t) > m · ATR_slow(t)` for some multiplier `m > 1`

The breakout direction is defined using the last closed bar relative to a short rolling price reference:
- **Bullish breakout** if `Close(t) > High(t-1)` (or above a short rolling max)
- **Bearish breakout** if `Close(t) < Low(t-1)` (or below a short rolling min)

This strategy combines:
1) volatility regime filter (only trade when volatility expands),
2) directional trigger (enter in the direction of the breakout).

### Trading rules (identical logic in both implementations)

The strategy is implemented as a finite-state trading system, with explicit signal evaluation and position state management.

#### Core state variables

`position_state ∈ {FLAT, LONG, SHORT}`  
Current exposure of the strategy. At most one position can be active at any time.

`ATR(t)`  
Fast ATR used to detect volatility expansion.

`ATR_slow(t)`  
Slow ATR baseline used to normalize volatility.

`expansion_flag(t)`  
Boolean condition: `ATR(t) > m · ATR_slow(t)`.

`breakout_up(t), breakout_down(t)`  
Directional triggers based on closed-bar breakout conditions.

`SL`, `TP` (optional)  
Fixed stop-loss and take-profit levels defined by the strategy at entry.  
The underlying risk logic is identical in both implementations, while the parameterization unit differs:
- in MQL5, `SL`/`TP` are expressed in points (platform-native convention) and attached directly to the order,
- in the standalone C++ program, `SL`/`TP` are expressed as percentages of the entry price and evaluated in code.

#### Signal definition (closed bars only)

**Volatility expansion at time t** if:  
`ATR(t) > m · ATR_slow(t)`

**Bullish breakout at time t** if:  
`Close(t) > High(t−1)`

**Bearish breakout at time t** if:  
`Close(t) < Low(t−1)`

Signals are computed on closed candles only to avoid intra-bar noise and ensure deterministic behavior.

#### State transition logic

**If expansion_flag(t) is true and breakout_up(t) is true:**
- if `position_state == SHORT` → close the short position
- if `position_state == FLAT` → open a long position

**If expansion_flag(t) is true and breakout_down(t) is true:**
- if `position_state == LONG` → close the long position
- if `position_state == FLAT` → open a short position

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
Risk evaluation is performed independently of the signal logic once in a position.

#### Implementation note

The strategy logic is strictly event-driven:
- indicators are computed from closed bars only,
- expansion and breakout signals are evaluated once per bar,
- position state transitions are explicit and deterministic,
- no parameter optimization, regime filtering beyond ATR expansion, or portfolio-level logic is applied at this stage.

This section describes the exact same logical structure implemented in both the MQL5 Expert Advisor and the standalone C++ program, despite differences in language syntax and execution environment.

## 6) Files
- `ATR_Expansion_Breakout.mq5`: MT5 Expert Advisor (market data + strategy tester)
- `ATR_Expansion_Breakout.cpp`: standalone C++ program (synthetic regime-switching data, same logic, full run)

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
