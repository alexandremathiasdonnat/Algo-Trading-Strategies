# Event / Macro / News-Driven - Strategy Pack (Macro Surprise Breakout)

## 1) Strategy Family: Event / Macro / News-Driven

### Core idea
*The alpha hypothesis is differential reaction to macroeconomic information.
Macro releases (CPI, NFP, rate decisions) create discontinuities: jumps / gaps, volatility bursts, regime switches in risk appetite.*

In systematic trading, “news-driven” is often less about continuous prediction and more about: reacting to discrete information shocks, conditioning exposure on macro regimes, exploiting calendar-based asymmetries.

### Main strategy archetypes in this family
- **News breakout**  
  Trade the macro surprise (CPI, NFP, FOMC), typically via a directional jump/volatility expansion mechanism.
- **Central bank day bias**  
  Specific behavior on central-bank days (FOMC/ECB/BoE), often modeled as volatility + directional asymmetry.
- **Macro regime filter**  
  Risk-on / risk-off classification used to scale or enable strategies.
- **Calendar effects**  
  Date-specific patterns (day-of-week, month-end, quarter-end, scheduled flows).

This folder implements one baseline representative strategy:
**Macro Surprise Breakout with Regime & Calendar Conditioning**.

## 2) Two independent components

News-driven systems normally require:
- timestamped real calendars,
- clean market data,
- realistic spread/slippage modeling.

Those pieces are intentionally out of scope here (no external feeds, no CSV, no networking).
Instead, this folder focuses on mechanical correctness in a controlled laboratory:

- a **synthetic macro-event market generator** (event study),
- a **news-breakout trading engine** combining:
  - surprise-based entries,
  - macro regime filtering,
  - central-bank day bias,
  - calendar conditioning.

Both programs are standalone C++ executables and fully autonomous.

**What this is:**
- A clean implementation of event-driven state, filters, and decision rules
- A foundation for later integration with real datasets/calendars

**What this is NOT:**
- Not an NLP news trading system
- Not a production macro execution pipeline
- Not a profitability claim

## 3) Synthetic macro-event market stream (event study)

The generator produces a price series with:
- normal periods (baseline volatility),
- scheduled macro events (CPI/NFP/FOMC-like),
- event windows where:
  - volatility increases,
  - jump probability increases,
  - returns include a “surprise-driven” jump component.

Each event has:
- an event type (regular macro vs central bank),
- a surprise value `surprise ~ N(0,1)` used to drive the jump sign and magnitude.

In addition, the generator includes a simple **risk-on / risk-off regime** that changes through time.

## 4) News breakout trading engine

The trading engine implements a minimal but realistic decomposition:

- **Signal**: trade the sign of macro surprise when |surprise| is large
- **Central bank day bias**: stronger sizing (and optional directional skew) on central bank events
- **Macro regime filter**: scale risk depending on risk-on / risk-off state
- **Calendar effects**: simple date effects (e.g., end-of-month / periodic flows) impacting sizing

The engine is explicit:
- event detection is deterministic (calendar known ex-ante),
- entries happen only at event timestamps,
- risk is managed via holding horizon and stop-like bounds.

## 5) Implemented strategy: Macro Surprise Breakout

### State variables

`position_state ∈ {FLAT, LONG, SHORT}`  
Current exposure. At most one position at a time.

`event_flag(t)`  
Whether `t` is an event timestamp.

`event_type(t) ∈ {MACRO, CENTRAL_BANK}`  
Macro release vs central bank day.

`surprise(t)`  
Event surprise value driving the breakout direction.

`regime(t) ∈ {RISK_ON, RISK_OFF}`  
Macro regime used to scale exposure.

`calendar_factor(t)`  
Simple periodic conditioning factor (e.g., flow days).

### Signal definition (event time only)

At an event timestamp `t`:

- **Go long** if `surprise(t) > +k`
- **Go short** if `surprise(t) < -k`
- Otherwise stay flat

This is a clean “news breakout” abstraction: direction comes from surprise sign, activity comes from surprise magnitude.

### Risk management (minimal baseline)

- positions have a fixed maximum holding horizon `H`,
- optional price-based exits (stop-like bounds) are evaluated in code.

The goal is mechanical correctness of event-driven logic, not performance optimization.

## 6) Files
- `event_study.cpp`: synthetic macro calendar + surprise-driven market generator
- `macro_news_breakout.cpp`: news breakout engine with regime & calendar conditioning (full run + reporting)

## 7) General Disclaimer 

A real consistent algorithmic trading strategy =

**Signal** (clear alpha source)  
\+ **Regime filter** (when the signal is active)  
\+ **Timing** (when to enter / exit)  
\+ **Risk management** (exposure limits, stops, drawdown control)

Many retail strategies fail because they:
- treat news candles like normal candles,
- ignore discontinuities (jumps) and volatility bursts,
- confuse “direction prediction” with “event risk handling”,
- underestimate transaction costs, spreads, slippage, and execution risk.

This repository focuses on mechanical correctness and conceptual clarity.
Performance claims and real-market execution are intentionally outside scope.Here, I deliberately focus on isolating and implementing one clear macro-event mechanism at a time.

---

***Alexandre Mathias DONNAT, Sr***
