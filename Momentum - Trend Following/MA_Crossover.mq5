//+------------------------------------------------------------------+
//|                                                   MA_Crossover.mq5|
//|            Trend-Following / Momentum — Moving Average Crossover  |
//|                              Proof-of-Work (independent strategy) |
//+------------------------------------------------------------------+
#property strict
#property version "1.00"

#include <Trade/Trade.mqh>
CTrade trade;

input group "Signal"
input int FastMAPeriod = 20;
input int SlowMAPeriod = 50;
input ENUM_MA_METHOD MaMethod = MODE_SMA;
input ENUM_APPLIED_PRICE PriceType = PRICE_CLOSE;

input group "Execution"
input double Lots = 0.10;
input bool OneDecisionPerBar = true;
input int SlippagePoints = 20;

input group "Risk (optional)"
input bool UseSLTP = true;
input int StopLossPoints = 500;     // points
input int TakeProfitPoints = 1000;  // points

int hFast = INVALID_HANDLE;
int hSlow = INVALID_HANDLE;
datetime lastBarTime = 0;

bool IsNewBar()
{
   datetime t = (datetime)iTime(_Symbol, _Period, 0);
   if(t != lastBarTime)
   {
      lastBarTime = t;
      return true;
   }
   return false;
}

bool GetMA(int handle, int shift, double &out)
{
   double buf[];
   if(CopyBuffer(handle, 0, shift, 1, buf) != 1) return false;
   out = buf[0];
   return true;
}

int PositionSide() // -1 none, 0 buy, 1 sell
{
   if(!PositionSelect(_Symbol)) return -1;
   long t = PositionGetInteger(POSITION_TYPE);
   if(t == POSITION_TYPE_BUY)  return 0;
   if(t == POSITION_TYPE_SELL) return 1;
   return -1;
}

void CloseSymbolPositions()
{
   for(int i = PositionsTotal() - 1; i >= 0; --i)
   {
      if(PositionSelectByIndex(i))
      {
         string sym = PositionGetString(POSITION_SYMBOL);
         if(sym == _Symbol)
         {
            ulong ticket = (ulong)PositionGetInteger(POSITION_TICKET);
            trade.PositionClose(ticket, SlippagePoints);
         }
      }
   }
}

void BuildStops(bool isBuy, double entry, double &sl, double &tp)
{
   sl = 0.0; tp = 0.0;
   if(!UseSLTP) return;

   double pt = _Point;
   if(isBuy)
   {
      sl = entry - StopLossPoints * pt;
      tp = entry + TakeProfitPoints * pt;
   }
   else
   {
      sl = entry + StopLossPoints * pt;
      tp = entry - TakeProfitPoints * pt;
   }
}

int OnInit()
{
   if(FastMAPeriod <= 0 || SlowMAPeriod <= 0 || FastMAPeriod >= SlowMAPeriod)
   {
      Print("Invalid MA params: require 0 < Fast < Slow");
      return INIT_FAILED;
   }

   hFast = iMA(_Symbol, _Period, FastMAPeriod, 0, MaMethod, PriceType);
   hSlow = iMA(_Symbol, _Period, SlowMAPeriod, 0, MaMethod, PriceType);
   if(hFast == INVALID_HANDLE || hSlow == INVALID_HANDLE)
   {
      Print("Failed to create MA handles.");
      return INIT_FAILED;
   }

   trade.SetDeviationInPoints(SlippagePoints);
   lastBarTime = (datetime)iTime(_Symbol, _Period, 0);
   return INIT_SUCCEEDED;
}

void OnDeinit(const int reason)
{
   if(hFast != INVALID_HANDLE) IndicatorRelease(hFast);
   if(hSlow != INVALID_HANDLE) IndicatorRelease(hSlow);
}

void OnTick()
{
   if(OneDecisionPerBar && !IsNewBar())
      return;

   // Use closed bars (shift 2 -> previous closed, shift 1 -> last closed)
   double f2, f1, s2, s1;
   if(!GetMA(hFast, 2, f2)) return;
   if(!GetMA(hFast, 1, f1)) return;
   if(!GetMA(hSlow, 2, s2)) return;
   if(!GetMA(hSlow, 1, s1)) return;

   bool bullishCross = (f2 <= s2) && (f1 > s1);
   bool bearishCross = (f2 >= s2) && (f1 < s1);

   int side = PositionSide();

   double ask = SymbolInfoDouble(_Symbol, SYMBOL_ASK);
   double bid = SymbolInfoDouble(_Symbol, SYMBOL_BID);

   if(bullishCross)
   {
      if(side == 0) return; // already long
      CloseSymbolPositions();
      double sl, tp; BuildStops(true, ask, sl, tp);
      trade.Buy(Lots, _Symbol, ask, sl, tp, "MA bullish crossover");
   }
   else if(bearishCross)
   {
      if(side == 1) return; // already short
      CloseSymbolPositions();
      double sl, tp; BuildStops(false, bid, sl, tp);
      trade.Sell(Lots, _Symbol, bid, sl, tp, "MA bearish crossover");
   }
}
