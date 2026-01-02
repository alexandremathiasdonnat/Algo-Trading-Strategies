# Mean Reversion / Range Trading - Strategy Pack (Bollinger Bands Reversion)

## 1) Strategy Family: Mean Reversion / Range Trading

### Core idea
*The alpha hypothesis is statistical mean reversion: after an excessive move away from a reference level, price has a non-zero probability of reverting back toward its mean.
Mean-reversion strategies try to fade extremes, accepting that strong trends can produce sequences of losses, in exchange for frequent smaller reversions.*

### Main strategy archetypes in this family
- **RSI Extremes**  
  Sell overbought conditions and buy oversold conditions, assuming short-term reversion.
- **Bollinger Bands Reversion**  
  Trade the return toward the middle band after price closes outside the upper/lower band.
- **Range Fade**  
  Sell the top of a range and buy the bottom, expecting repeated oscillations.
- **VWAP Reversion**  
  Trade reversion of price toward a volume-weighted average reference.

This folder implements one baseline representative strategy: **Bollinger Bands Reversion**.

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

## 5) The dual implemented strategy: Bollinger Bands Reversion

### Signal definition

Let:
- `BB_upper(t)`, `BB_middle(t)`, `BB_lower(t)` be Bollinger Bands computed on Close prices (period `N`, standard deviation multiplier `k`)
- `P(t)` be the close price of the last closed bar

A mean-reversion entry signal is detected on **closed bars**:
- **Overbought extreme (short setup)** if `P(t) > BB_upper(t)`
- **Oversold extreme (long setup)** if `P(t) < BB_lower(t)`

The strategy assumes that after an “excess” move outside the bands, price has a non-zero probability of reverting toward the **middle band**.

### Trading rules (identical logic in both implementations)

The strategy is implemented as a finite-state trading system, with explicit signal evaluation and position state management.

#### Core state variables

`position_state ∈ {FLAT, LONG, SHORT}`  
Current exposure of the strategy. At most one position can be active at any time.

`BB_upper(t), BB_middle(t), BB_lower(t)`  
Bollinger Bands computed on close prices over a rolling window.

`P(t)`  
Close price of the most recent closed bar (signal evaluated on closed candles only).

`SL`, `TP` (optional)  
Fixed stop-loss and take-profit levels defined by the strategy at entry.  
The underlying risk logic is identical in both implementations, while the parameterization unit differs:
- in MQL5, `SL`/`TP` are expressed in points (platform-native convention) and attached directly to the order,
- in the standalone C++ program, `SL`/`TP` are expressed as percentages of the entry price and evaluated in code.

#### Signal definition

A mean-reversion setup is detected on closed bars only:

**Short setup at time t** if:  
`P(t) > BB_upper(t)`

**Long setup at time t** if:  
`P(t) < BB_lower(t)`

This formulation avoids intra-bar noise and ensures deterministic signal generation.

#### State transition logic

**On a short setup:**
- if `position_state == LONG` → close the long position
- if `position_state == FLAT` → open a short position

**On a long setup:**
- if `position_state == SHORT` → close the short position
- if `position_state == FLAT` → open a long position

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
Risk evaluation is performed independently of the entry signal.

#### Implementation note

The strategy logic is strictly event-driven:
- signals are evaluated once per closed bar,
- position state transitions are explicit and deterministic,
- no parameter optimization, regime filtering, or portfolio-level logic is applied at this stage.

This section describes the exact same logical structure implemented in both the MQL5 Expert Advisor and the standalone C++ program, despite differences in language syntax and execution environment.

## 6) Files
- `BB_Reversion.mq5`: MT5 Expert Advisor (market data + strategy tester)
- `BB_Reversion.cpp`: standalone C++ program (synthetic data, same logic, full run)

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