# Trend-Following / Momentum — Strategy Pack (MA Crossover)

## 1) Family: Trend-Following / Momentum

### Core idea
The alpha hypothesis is **directional persistence**: once a move starts, it has a non-zero probability of continuing.
Trend-following strategies try to **enter late but ride the trend**, accepting many small losses in exchange for occasional large trend gains.

### Main strategy archetypes in this family
- **Moving Average Crossover**  
  Enter when a fast moving average crosses a slow one, signaling a potential trend start.
- **Channel / Donchian Breakout**  
  Trade the breakout above/below a multi-period price channel.
- **Time-Series Momentum**  
  Go long if past returns are positive, short if negative (often with a fixed lookback).
- **ADX / Momentum Filters**  
  Only trade when trend strength (or momentum) exceeds a threshold, filtering sideways regimes.

This folder implements **one baseline representative strategy**: **Moving Average Crossover**.

---

## 2) Why two independent implementations?

This repository is a **proof-of-work** exercise:
- demonstrate that I can implement the *same trading logic* in **two different languages**
- keep the implementations **fully independent** (no bridge, no shared data pipeline)

### ✅ What this is
- A demonstration of **clean strategy abstraction**
- A demonstration of **algorithmic consistency across languages**
- A practical way to build intuition on execution details, state management, and risk logic

### ❌ What this is NOT
- Not a production trading architecture
- Not a live execution system
- Not a claim that this strategy is profitable as-is

---

## 3) Why MT5 / MQL5 for the first implementation?

MetaTrader 5 is a practical sandbox for retail quant experimentation:
- built-in access to **market data** through the broker environment (demo accounts are easy to obtain)
- native **strategy tester** for quick iterations
- integrated execution model (orders, positions, symbol properties, spreads, etc.)
- fast feedback loop: code → compile → backtest → iterate

So the `.mq5` version is the most direct way to test the strategy on real historical market conditions (within MT5’s limits).

---

## 4) Why a standalone C++ implementation?

A standalone C++ program forces me to re-implement the strategy logic **from first principles**:
- signals and state transitions (flat → long/short → flip)
- risk logic (SL/TP)
- trade bookkeeping and performance reporting

Because this project has strict constraints (no CSV / no external feed / no networking), the C++ version runs on an **internally generated synthetic price series** (GBM-like random walk).  

This is deliberate:
- it keeps the code 100% autonomous
- it demonstrates the full end-to-end logic without relying on external data plumbing
- it sets up a clean base for later projects where I may plug real data properly (actions/FX/crypto datasets, custom backtest engine, etc.)

---

## 5) Implemented strategy: Moving Average Crossover (baseline)

### Signal definition
Let:
- `MA_fast(t)` be the fast moving average of Close prices (e.g., 20)
- `MA_slow(t)` be the slow moving average of Close prices (e.g., 50)

A crossover is detected on **closed bars**:
- **Bullish crossover**: `MA_fast` crosses from below to above `MA_slow`
- **Bearish crossover**: `MA_fast` crosses from above to below `MA_slow`

### Trading rules (same in both codes)
- If bullish crossover:
  - if currently short → close short
  - if flat → open long
- If bearish crossover:
  - if currently long → close long
  - if flat → open short
- At most **one position at a time**
- Optional fixed **Stop-Loss / Take-Profit**

### Implementation notes
- The signal is computed using **closed candles** to avoid intra-bar noise and repainting.
- The logic is intentionally minimal: this is a baseline trend-following template,
  meant to be extended later with regime filters, cost modeling, and robustness tests.

---

## Files
- `MA_Crossover.mq5` : MT5 Expert Advisor (market data + strategy tester)
- `MA_Crossover.cpp` : standalone C++ program (synthetic data, same logic, full run)
