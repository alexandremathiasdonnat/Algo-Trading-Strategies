# Statistical Arbitrage - Strategy Pack (Pairs Trading / Cointegration)

## 1) Strategy Family: Statistical Arbitrage

### Core idea
*The alpha hypothesis is relative mispricing: while individual asset prices are noisy and non-stationary, some asset pairs exhibit stable long-run relationships.*
*Statistical arbitrage strategies exploit temporary deviations from an equilibrium relationship, assuming mean-reversion of the spread.*

*Unlike directional strategies, the core object is not the price, but a stationary transformation of prices (spread).*

### Main strategy archetypes in this family
- **Pairs Trading (cointegration / spread mean-reversion)**  
  Trade deviations of a stationary spread around its equilibrium.
- **Cross-sectional mean reversion**  
  Long the underperformers and short the outperformers within a universe.
- **Basket / factor-neutral statarb**  
  Trade residuals after hedging common factor exposures.
- **Kalman / time-varying hedge ratio**  
  Adaptive estimation of the hedge ratio β(t) and spread dynamics.

This folder implements one baseline representative strategy: **Pairs Trading (rolling OLS hedge ratio + z-score spread)**.

## 2) Two independent components

This folder follows the same proof-of-work philosophy: simple, autonomous, and mechanically correct implementations.

It is split into:
- a **synthetic generator** producing a controlled cointegrated pair,
- a **pairs trading engine** implementing the full end-to-end strategy logic.

There is:
- no external dataset,
- no CSV,
- no networking,
- no hidden research pipeline.

The goal is to demonstrate:
- correct spread construction (hedged portfolio),
- z-score signal generation without lookahead,
- explicit state transitions,
- explicit PnL and trade bookkeeping.

**What this is:**
- A clean implementation of a statarb baseline
- A deterministic mechanical backtest on a controlled environment
- A strong foundation for later research extensions

**What this is NOT:**
- Not a production statarb system
- Not a claim of real-world profitability
- Not a complete cost/slippage/funding model

## 3) Synthetic cointegrated pair

A classic way to simulate cointegration is:
- one latent random walk `X(t)`,
- a second series `Y(t) = β·X(t) + noise(t)`.

Then the spread:
`S(t) = Y(t) − β·X(t)`
is (approximately) stationary.

The generator also allows regime-like changes in noise intensity, to test robustness.

## 4) Pairs trading engine

The trading engine operates on closed observations and follows these steps:

1. **Estimate hedge ratio**  
   Rolling OLS regression of `Y` on `X` over a lookback window `L`:
   `Y ≈ α + βX`.

2. **Compute spread and z-score**  
   `S(t) = Y(t) − (α + βX(t))`  
   z-score computed using rolling mean and std of the spread:
   `Z(t) = (S(t) − μ_S) / σ_S`.

3. **Trading rule (mean reversion)**  
   - if `Z(t) > entry` → short spread (short Y, long βX)
   - if `Z(t) < -entry` → long spread (long Y, short βX)
   - exit when `|Z(t)| < exit`.

4. **PnL bookkeeping**  
   PnL is computed on the hedged portfolio:
   `PnL(t) = wY·ΔY + wX·ΔX`
   with weights consistent with the spread direction.

All decisions are made using information available up to time `t` only (no lookahead).

## 5) Implemented strategy: Pairs Trading (rolling OLS + z-score)

### State variables

`position_state ∈ {FLAT, LONG_SPREAD, SHORT_SPREAD}`  
At most one spread position at a time.

`β(t), α(t)`  
Rolling OLS estimates of hedge ratio and intercept.

`S(t)`  
Spread value.

`Z(t)`  
Z-score of the spread over a rolling window.

### Signal definition

Mean-reversion entry:
- **Short spread** if `Z(t) > z_entry`
- **Long spread** if `Z(t) < -z_entry`

Exit:
- close position if `|Z(t)| < z_exit`

### Risk management (minimal baseline)

A time-stop is included:
- if a position lasts more than `max_hold` steps, it is closed.

Stop-loss / take-profit can be layered later but are not the focus of this baseline.

## 6) Files
- `synthetic_pairs.cpp`: cointegrated pair generator
- `pairs_trading.cpp`: rolling OLS + z-score trading engine (full run + reporting)

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

***Alexandre Mathias DONNAT***