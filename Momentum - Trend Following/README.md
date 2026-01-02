# Trend-Following / Momentum - Strategy Pack (MA Crossover)

## 1) Strategy Family: Trend-Following / Momentum

### Core idea
The alpha hypothesis is directional persistence: once a move starts, it has a non-zero probability of continuing.
Trend-following strategies try to enter late but ride the trend, accepting many small losses in exchange for occasional large trend gains.

### Main strategy archetypes in this family
- **Moving Average Crossover**  
  Enter when a fast moving average crosses a slow one, signaling a potential trend start.
- **Channel / Donchian Breakout**  
  Trade the breakout above/below a multi-period price channel.
- **Time-Series Momentum**  
  Go long if past returns are positive, short if negative (often with a fixed lookback).
- **ADX / Momentum Filters**  
  Only trade when trend strength (or momentum) exceeds a threshold, filtering sideways regimes.

This folder implements one baseline representative strategy: **Moving Average Crossover**.

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

- explicit signal computation,
- explicit position state management (flat → long / short → flip),
- explicit risk logic (stop-loss / take-profit),
- explicit trade tracking and performance aggregation.

Because this is a standalone program with no dependency on broker infrastructure, the strategy is executed on an internally generated synthetic price series (GBM-like random walk).

This choice is deliberate and meaningful:
- it keeps the code fully autonomous and self-contained,
- it avoids hiding logic behind platform-specific abstractions,
- it highlights the core mechanics of the strategy rather than data plumbing,
- it mirrors how quantitative research often starts with simplified or simulated environments before scaling to real datasets.

This C++ version therefore acts as a research-oriented counterpart to the MT5 implementation, and as a clean foundation for future extensions (e.g. custom backtesting engines, large-scale simulations, or real-data integration in separate projects).

## 5) The dual implemented strategy: Moving Average Crossover

### Signal definition

Let:
- `MA_fast(t)` be the fast moving average of Close prices (e.g., 20-period)
- `MA_slow(t)` be the slow moving average of Close prices (e.g., 50-period)

A crossover is detected on closed bars:
- **Bullish crossover**: `MA_fast` crosses from below to above `MA_slow`
- **Bearish crossover**: `MA_fast` crosses from above to below `MA_slow`

### Trading rules (identical logic in both implementations)

The strategy is implemented as a finite-state trading system, with explicit signal evaluation and position state management.

#### Core state variables

`position_state ∈ {FLAT, LONG, SHORT}`  
Current exposure of the strategy. At most one position can be active at any time.

`MA_fast(t)`  
Fast moving average computed on close prices over a short rolling window.

`MA_slow(t)`  
Slow moving average computed on close prices over a longer rolling window.

`SL`, `TP` (optional)  
Fixed stop-loss and take-profit levels defined by the strategy at entry.  
The underlying risk logic is identical in both implementations, while the parameterization unit differs:
- in MQL5, `SL`/`TP` are expressed in points (platform-native convention) and attached directly to the order,
- in the standalone C++ program, `SL`/`TP` are expressed as percentages of the entry price and evaluated in code.

#### Signal definition

A crossover event is detected on closed bars only:

**Bullish crossover at time t** if:  
`MA_fast(t−1) ≤ MA_slow(t−1)` **and** `MA_fast(t) > MA_slow(t)`

**Bearish crossover at time t** if:  
`MA_fast(t−1) ≥ MA_slow(t−1)` **and** `MA_fast(t) < MA_slow(t)`

This formulation avoids intra-bar noise and ensures deterministic signal generation.

#### State transition logic

**On a bullish crossover:**
- if `position_state == SHORT` → close the short position
- if `position_state == FLAT` → open a long position

**On a bearish crossover:**
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
Risk evaluation is performed independently of crossover events.

#### Implementation note

The strategy logic is strictly event-driven:
- signals are evaluated once per closed bar,
- position state transitions are explicit and deterministic,
- no parameter optimization, regime filtering, or portfolio-level logic is applied at this stage.

This section describes the exact same logical structure implemented in both the MQL5 Expert Advisor and the standalone C++ program, despite differences in language syntax and execution environment.

## 6) Files
- `MA_Crossover.mq5`: MT5 Expert Advisor (market data + strategy tester)
- `MA_Crossover.cpp`: standalone C++ program (synthetic data, same logic, full run)

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
