# Execution / Market Impact - Execution Engine Pack

## 1) Strategy Family: Execution / Market Impact

### Core idea
*The alpha does not come from predicting price direction, but from how orders are executed.*

*Execution strategies aim to: minimize market impact, reduce slippage, control transaction costs, transform theoretical signals into realizable PnL.*

*In professional trading systems, execution is a separate layer from signal generation.*

### Main strategy archetypes in this family
- **TWAP / VWAP execution**  
  Slice a large order over time to reduce footprint.
- **Smart order routing**  
  Route orders to venues offering best liquidity and price.
- **Slippage control**  
  Model and limit invisible costs (impact, spread, volatility).

This folder focuses on **execution mechanics only**, independent of any trading signal.

## 2) Scope and design philosophy

This folder is intentionally different from the others.

Execution is not about:
- deciding when to buy or sell,
- generating alpha signals.

Execution is about:
- *how* a desired position is built or unwound,
- *how much* it really costs to trade.

Therefore:
- no MQL5 implementation is provided,
- no broker platform is used,
- no price prediction is involved.

This reflects real-world architecture:
signal research and execution engines are **separate systems**.

## 3) Standalone C++ execution engines

Both programs in this folder are fully autonomous C++ simulations.

They:
- generate their own synthetic market conditions,
- simulate order slicing and fills,
- track execution cost, slippage, and impact.

They are not backtests of trading strategies,
but controlled experiments on execution quality.

## 4) TWAP execution engine

The TWAP engine simulates the execution of a large parent order by:
- splitting it into equal-sized child orders,
- executing them at fixed time intervals,
- aggregating realized execution cost.

This isolates the effect of:
- execution horizon,
- order size,
- volatility,
- temporary impact.

## 5) Slippage and impact modeling

The slippage model introduces:
- stochastic price moves,
- spread costs,
- impact proportional to order size and volatility.

This allows explicit measurement of:
- expected execution cost,
- variance of execution outcomes,
- sensitivity to market conditions.

## 6) Files
- `twap_execution.cpp`: time-sliced execution simulator (TWAP)
- `slippage_model.cpp`: impact and slippage modeling engine

## 7) General Disclaimer

In real trading systems:

**Signal generation**  
≠ **Risk management**  
≠ **Execution**

Many retail traders ignore execution entirely,
assuming fills occur at theoretical prices.

In reality, execution quality is often the difference between: a profitable strategy, and an untradeable one. This folder focuses exclusively on execution mechanics,
independent of signal quality or profitability claims.

---

***Alexandre Mathias DONNAT, Sr***
