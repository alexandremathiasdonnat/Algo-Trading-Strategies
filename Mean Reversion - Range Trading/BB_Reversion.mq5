//+------------------------------------------------------------------+
//|                                                   BB_Reversion.mq5|
//|                    Mean Reversion / Range Trading (Proof-of-Work) |
//|                    Alexandre Mathias DONNAT - Independent Strategy|
//+------------------------------------------------------------------+
#property strict
#property version   "1.00"

#include <Trade/Trade.mqh>
CTrade trade;

input group "Bollinger Bands"
input ENUM_TIMEFRAMES bbTimeframe = PERIOD_H4;
input int            bbPeriod     = 20;
input double         bbStd        = 2.0;
input ENUM_APPLIED_PRICE bbPrice  = PRICE_CLOSE;

input group "Trend Filter (optional)"
input bool           useTrendFilter = true;
input ENUM_TIMEFRAMES maTimeframe   = PERIOD_H4;
input int            maPeriod       = 200;
input ENUM_MA_METHOD maMethod       = MODE_SMA;
input ENUM_APPLIED_PRICE maPrice    = PRICE_CLOSE;

input group "Risk / Trade"
input int            stopLossPoints  = 200;
input int            takeProfitPoints= 200;
input double         riskPct         = 0.5;   // % of balance risked per trade
input bool           allowLong       = true;
input bool           allowShort      = true;

int bbHandle = INVALID_HANDLE;
int maHandle = INVALID_HANDLE;

int lastBars = 0;
bool isTradeOpen = false;

double SL_dist, TP_dist;

//--- helper: compute lot size from SL distance (in price units)
double GetPositionSize(double slDistancePrice)
{
   double balance   = AccountInfoDouble(ACCOUNT_BALANCE);
   double riskMoney = balance * (riskPct / 100.0);

   double tickSize  = SymbolInfoDouble(_Symbol, SYMBOL_TRADE_TICK_SIZE);
   double tickValue = SymbolInfoDouble(_Symbol, SYMBOL_TRADE_TICK_VALUE);

   double volStep   = SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_STEP);
   double volMin    = SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_MIN);
   double volMax    = SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_MAX);

   // money risked per 1 volumeStep for given SL distance
   double riskPerStep = (slDistancePrice / tickSize) * tickValue * volStep;
   if(riskPerStep <= 0.0) return volMin;

   double rawVol = MathFloor(riskMoney / riskPerStep) * volStep;
   if(rawVol < volMin) rawVol = volMin;
   if(rawVol > volMax) rawVol = volMax;

   return rawVol;
}

//--- helper: check if we currently hold a position on this symbol
bool HasOpenPosition()
{
   for(int i=0;i<PositionsTotal();i++)
   {
      if(PositionGetSymbol(i) == _Symbol) return true;
   }
   return false;
}

int OnInit()
{
   bbHandle = iBands(_Symbol, bbTimeframe, bbPeriod, 0, bbStd, bbPrice);
   if(bbHandle == INVALID_HANDLE)
      return(INIT_FAILED);

   if(useTrendFilter)
   {
      maHandle = iMA(_Symbol, maTimeframe, maPeriod, 0, maMethod, maPrice);
      if(maHandle == INVALID_HANDLE)
         return(INIT_FAILED);
   }

   SL_dist = stopLossPoints * _Point;
   TP_dist = takeProfitPoints * _Point;

   return(INIT_SUCCEEDED);
}

void OnDeinit(const int reason)
{
   if(bbHandle != INVALID_HANDLE) IndicatorRelease(bbHandle);
   if(maHandle != INVALID_HANDLE) IndicatorRelease(maHandle);
}

void OnTick()
{
   // operate only once per new bar on the BB timeframe (closed candle logic)
   int barsTotal = iBars(_Symbol, bbTimeframe);
   if(barsTotal <= 2) return;

   if(barsTotal == lastBars) return;
   lastBars = barsTotal;

   // refresh position flag
   isTradeOpen = HasOpenPosition();

   // fetch last closed bar close price on bb timeframe
   double close1 = iClose(_Symbol, bbTimeframe, 1);
   if(close1 <= 0.0) return;

   // buffers (index 1 is last closed bar)
   double bbUpper[2], bbLower[2], bbMiddle[2];
   if(CopyBuffer(bbHandle, UPPER_BAND, 1, 1, bbUpper) <= 0) return;
   if(CopyBuffer(bbHandle, LOWER_BAND, 1, 1, bbLower) <= 0) return;
   if(CopyBuffer(bbHandle, BASE_LINE,  1, 1, bbMiddle)<= 0) return;

   double maVal = 0.0;
   if(useTrendFilter)
   {
      double maBuf[1];
      if(CopyBuffer(maHandle, 0, 1, 1, maBuf) <= 0) return;
      maVal = maBuf[0];
   }

   double ask = SymbolInfoDouble(_Symbol, SYMBOL_ASK);
   double bid = SymbolInfoDouble(_Symbol, SYMBOL_BID);

   // Trend filter (optional): for mean reversion, keep it simple:
   // - if price is below MA, prefer short setups (sell overbought)
   // - if price is above MA, prefer long setups (buy oversold)
   bool allowShortHere = allowShort;
   bool allowLongHere  = allowLong;

   if(useTrendFilter)
   {
      allowShortHere = allowShort && (bid < maVal);
      allowLongHere  = allowLong  && (ask > maVal);
   }

   // ENTRY LOGIC (1 position max)
   // Short: close above upper band
   if(!isTradeOpen && allowShortHere && close1 > bbUpper[0])
   {
      double sl = ask + SL_dist;
      double tp = bid - TP_dist;

      double vol = GetPositionSize(SL_dist);
      trade.Sell(vol, _Symbol, bid, sl, tp);
      return;
   }

   // Long: close below lower band
   if(!isTradeOpen && allowLongHere && close1 < bbLower[0])
   {
      double sl = bid - SL_dist;
      double tp = ask + TP_dist;

      double vol = GetPositionSize(SL_dist);
      trade.Buy(vol, _Symbol, ask, sl, tp);
      return;
   }
}
