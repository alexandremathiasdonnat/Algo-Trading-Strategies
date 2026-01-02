//+------------------------------------------------------------------+
//|                                      ATR_Expansion_Breakout.mq5  |
//|                                     Volatility-Based Strategies  |
//|                 Alexandre Mathias DONNAT - Independent strategy  |
//+------------------------------------------------------------------+
#property strict
#property version   "1.00"

#include <Trade/Trade.mqh>
CTrade trade;

input group "Timeframe"
input ENUM_TIMEFRAMES tf = PERIOD_H1;

input group "ATR (fast/slow)"
input int atrFastPeriod = 14;
input int atrSlowPeriod = 50;
input double expansionMult = 1.50;   // trigger if ATR_fast > mult * ATR_slow

input group "Breakout trigger"
input int breakoutLookback = 1;      // simplest: compare Close(t) to High/Low(t-1)

input group "Risk / Trade"
input bool   useRiskSizing = false;
input double fixedLots     = 0.50;
input double riskPct       = 0.50;
input int    stopLossPoints   = 250;
input int    takeProfitPoints = 500;
input bool   allowLong  = true;
input bool   allowShort = true;

int atrFastHandle = INVALID_HANDLE;
int atrSlowHandle = INVALID_HANDLE;

int lastBars = 0;

double SL_dist = 0.0;
double TP_dist = 0.0;

double GetPositionSize(double slDistancePrice)
{
   if(!useRiskSizing) return fixedLots;

   double balance   = AccountInfoDouble(ACCOUNT_BALANCE);
   double riskMoney = balance * (riskPct / 100.0);

   double tickSize  = SymbolInfoDouble(_Symbol, SYMBOL_TRADE_TICK_SIZE);
   double tickValue = SymbolInfoDouble(_Symbol, SYMBOL_TRADE_TICK_VALUE);

   double volStep   = SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_STEP);
   double volMin    = SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_MIN);
   double volMax    = SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_MAX);

   double riskPerStep = (slDistancePrice / tickSize) * tickValue * volStep;
   if(riskPerStep <= 0.0) return volMin;

   double vol = MathFloor(riskMoney / riskPerStep) * volStep;
   if(vol < volMin) vol = volMin;
   if(vol > volMax) vol = volMax;
   return vol;
}

int OnInit()
{
   atrFastHandle = iATR(_Symbol, tf, atrFastPeriod);
   atrSlowHandle = iATR(_Symbol, tf, atrSlowPeriod);

   if(atrFastHandle == INVALID_HANDLE || atrSlowHandle == INVALID_HANDLE)
      return(INIT_FAILED);

   SL_dist = stopLossPoints * _Point;
   TP_dist = takeProfitPoints * _Point;

   return(INIT_SUCCEEDED);
}

void OnDeinit(const int reason)
{
   if(atrFastHandle != INVALID_HANDLE) IndicatorRelease(atrFastHandle);
   if(atrSlowHandle != INVALID_HANDLE) IndicatorRelease(atrSlowHandle);
}

void OnTick()
{
   // Run once per new bar on timeframe tf (closed bar logic)
   int barsTotal = iBars(_Symbol, tf);
   if(barsTotal <= atrSlowPeriod + 5) return;
   if(barsTotal == lastBars) return;
   lastBars = barsTotal;

   // If we already have a position on this symbol, do nothing (SL/TP handled by broker)
   bool hasPos = PositionSelect(_Symbol);

   // Get last closed bar prices
   double c1 = iClose(_Symbol, tf, 1);
   double h2 = iHigh (_Symbol, tf, 2); // High(t-1)
   double l2 = iLow  (_Symbol, tf, 2); // Low(t-1)
   if(c1 <= 0.0) return;

   // ATR buffers on last closed bar
   double atrF[1], atrS[1];
   if(CopyBuffer(atrFastHandle, 0, 1, 1, atrF) <= 0) return;
   if(CopyBuffer(atrSlowHandle, 0, 1, 1, atrS) <= 0) return;

   bool expansion = (atrF[0] > expansionMult * atrS[0]);

   // Directional breakout trigger on closed bar
   bool breakoutUp   = (c1 > h2);
   bool breakoutDown = (c1 < l2);

   if(!expansion) return; // regime filter

   double ask = SymbolInfoDouble(_Symbol, SYMBOL_ASK);
   double bid = SymbolInfoDouble(_Symbol, SYMBOL_BID);
   double vol = GetPositionSize(SL_dist);

   // If no position: open on breakout
   if(!hasPos)
   {
      if(allowLong && breakoutUp)
      {
         trade.Buy(vol, _Symbol, ask, bid - SL_dist, ask + TP_dist);
         return;
      }
      if(allowShort && breakoutDown)
      {
         trade.Sell(vol, _Symbol, bid, ask + SL_dist, bid - TP_dist);
         return;
      }
      return;
   }

   // If position exists, we keep it simple: do not flip on signal
   
}
