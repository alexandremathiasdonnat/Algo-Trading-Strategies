# Machine Learning / Adaptive Strategies - Strategy Pack (Online Expert Aggregation)

## 1) Strategy Family: Machine Learning / Adaptive Strategies

### Core idea
*The alpha hypothesis is adaptability: markets are non-stationary, and no single signal performs well across all regimes.
Adaptive strategies aim to dynamically reweight or select signals based on their recent performance, instead of relying on fixed parameters or static models.*

### Main strategy archetypes in this family
- **Feature-based ML**  
  Directional or probabilistic prediction using engineered features.
- **Regime classification**  
  Detecting market regimes (trend / range / high volatility) and switching behavior.
- **Ensemble / Mixture of Experts (MoE)**  
  Combining multiple strategies or models with dynamic weights.
- **Online learning**  
  Continuously updating model parameters or weights as new data arrives.

This folder implements one baseline representative strategy: **Online Expert Aggregation (Mixture of Experts)**.

## 2) Two independent components

This folder differs slightly from pure directional strategy packs.

Instead of implementing the same logic in two execution environments, the structure is split into:
- a **synthetic market generator**, and
- an **adaptive decision engine**.

Both components are written in C++ and fully autonomous.

There is:
- no external data feed,
- no CSV,
- no offline training,
- no hidden state.

The objective is to demonstrate:
- online learning mechanics,
- adaptive signal combination,
- deterministic and bias-free implementation.

**What this is:**
- A clean implementation of online learning / expert aggregation
- A controlled environment to study adaptive behavior
- A realistic abstraction of how adaptive models are built in practice

**What this is NOT:**
- Not a production ML trading system
- Not a claim of predictive superiority
- Not a performance-optimized research pipeline

## 3) Synthetic market stream

The market stream is generated internally and exhibits:
- alternating regimes (trend, mean-reversion, noise),
- changing volatility levels,
- non-stationary behavior.

This allows testing whether an adaptive system:
- increases weight on the right expert,
- reduces exposure to underperforming signals,
- reacts smoothly to regime changes.

The synthetic generator plays the role of a controlled laboratory.

## 4) Online expert aggregation engine

The adaptive engine combines several predefined experts:
- a trend-following expert,
- a mean-reversion expert,
- a random/noise expert (baseline).

Each expert produces a directional signal at each time step.

The aggregation layer:
- assigns a weight to each expert,
- updates these weights online based on realized outcomes,
- produces a single aggregated trading signal.

This corresponds to a **Mixture of Experts** or **online portfolio of strategies**.

## 5) Implemented strategy: Online Expert Aggregation

### Experts

Let:
- `E₁(t)` be a trend-following signal,
- `E₂(t)` be a mean-reversion signal,
- `E₃(t)` be a random baseline signal.

Each expert outputs a signal in `{−1, 0, +1}`.

### Aggregation logic

The aggregated signal is: S(t) = sign( Σ wᵢ(t) · Eᵢ(t) )


where:
- `wᵢ(t)` are adaptive weights,
- `Σ wᵢ(t) = 1`.

### Online weight update

After observing the realized return `r(t)`:
- experts aligned with the outcome are rewarded,
- misaligned experts are penalized.

Weights are updated using a multiplicative update rule: wᵢ(t+1) ∝ wᵢ(t) · exp(η · Eᵢ(t) · r(t))


where `η` is a learning rate.

Weights are then renormalized.

This is conceptually close to:
- Hedge algorithm,
- Exponential Weights,
- Online Mixture of Experts.

## 6) Files
- `synthetic_market_stream.cpp`: synthetic non-stationary market generator
- `online_expert_aggregation.cpp`: adaptive online learning engine

## 7) General Disclaimer 

A real consistent algorithmic trading strategy =

**Signal** (clear alpha source)  
\+ **Regime awareness** (when the signal is relevant)  
\+ **Timing** (how and when to act)  
\+ **Risk management** (exposure, drawdown control)

Many retail ML strategies fail because they:
- treat ML as a black box,
- overfit static datasets,
- ignore non-stationarity,
- confuse prediction with decision-making.

This repository focuses on **mechanical and logical correctness**:
each strategy is implemented to verify that adaptive behavior, state updates, and learning rules are correct, deterministic, and bias-free.

Performance optimization, feature engineering, and large-scale evaluation are intentionally outside the scope of this repository.

Here, I deliberately focus on isolating and implementing one clear adaptive mechanism at a time.

---

***Alexandre Mathias DONNAT, Sr***
