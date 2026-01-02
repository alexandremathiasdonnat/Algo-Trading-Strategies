//+------------------------------------------------------------------+
//|                                                London_Breakout.mq5|
//|                                       Session-Based / Time-of-Day |
//|                    Alexandre Mathias DONNAT - Independent Strategy|
//+------------------------------------------------------------------+
#property strict
#property version   "1.00"

#include <Trade/Trade.mqh>
CTrade trade;

input group "Session Times (server time)"
input int  asiaStartHour   = 0;   // inclusive
input int  asiaEndHour     = 8;   // exclusive (range computed up to this hour)
input int  londonOpenHour  = 9;   // place orders at this hour
input int  londonCloseHour = 18;  // pending orders expire at this hour

input group "Range Computation"
input ENUM_TIMEFRAMES rangeTF = PERIOD_H1; // timeframe used for Asian range
input double bufferPoints = 0;            // optional buffer beyond high/low

input group "Risk / Trade"
input bool   useRiskSizing = false;
input double fixedLots     = 0.50;
input double riskPct       = 0.50;   // % of balance risked per trade (if useRiskSizing)
input int    stopLossPoints= 200;
input int    takeProfitPoints=400;

datetime londonOpenTime=0, londonCloseTime=0;
int lastDayKey = -1;     // YYYYMMDD integer
bool ordersPlacedToday = false;

double SL_dist = 0.0;
double TP_dist = 0.0;

//--- helper: day key YYYYMMDD in server time
int DayKey(datetime t)
{
   MqlDateTime s; TimeToStruct(t, s);
   return s.year*10000 + s.mon*100 + s.day;
}

//--- helper: build a datetime for "today at hour:00:00" in server time
datetime TodayAtHour(datetime now, int hour)
{
   MqlDateTime s; TimeToStruct(now, s);
   s.hour = hour; s.min = 0; s.sec = 0;
   return StructToTime(s);
}

//--- helper: risk-based sizing from SL distance (price units)
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

//--- helper: cancel all pending orders on this symbol
void CancelPending()
{
   for(int i=OrdersTotal()-1; i>=0; --i)
   {
      if(!OrderSelect(i, SELECT_BY_POS, MODE_TRADES)) continue;
      if(OrderGetString(ORDER_SYMBOL) != _Symbol) continue;

      ENUM_ORDER_TYPE t = (ENUM_ORDER_TYPE)OrderGetInteger(ORDER_TYPE);
      if(t==ORDER_TYPE_BUY_STOP || t==ORDER_TYPE_SELL_STOP ||
         t==ORDER_TYPE_BUY_LIMIT|| t==ORDER_TYPE_SELL_LIMIT)
      {
         ulong ticket = (ulong)OrderGetInteger(ORDER_TICKET);
         trade.OrderDelete(ticket);
      }
   }
}

//--- helper: compute Asian range high/low using closed bars strictly before asiaEndHour
bool ComputeAsianRange(datetime now, double &asiaHigh, double &asiaLow)
{
   datetime dayStart = TodayAtHour(now, 0);
   datetime startT = TodayAtHour(now, asiaStartHour);
   datetime endT   = TodayAtHour(now, asiaEndHour);

   // If asiaEndHour < asiaStartHour, we assume it wraps midnight (rare). Keep simple: not supported here.
   if(endT <= startT) return false;

   // iterate over bars on rangeTF and include those with open time in [startT, endT)
   int bars = iBars(_Symbol, rangeTF);
   if(bars < 5) return false;

   asiaHigh = -DBL_MAX;
   asiaLow  =  DBL_MAX;

   for(int shift=1; shift<bars; ++shift) // start at 1 (closed bars)
   {
      datetime bt = iTime(_Symbol, rangeTF, shift);
      if(bt < startT) break;         // gone before window
      if(bt >= endT) continue;       // after window
      double h = iHigh(_Symbol, rangeTF, shift);
      double l = iLow(_Symbol, rangeTF, shift);
      if(h > asiaHigh) asiaHigh = h;
      if(l < asiaLow)  asiaLow  = l;
   }

   if(asiaHigh <= -DBL_MAX/2 || asiaLow >= DBL_MAX/2) return false;
   if(asiaHigh <= asiaLow) return false;
   return true;
}

int OnInit()
{
   SL_dist = stopLossPoints * _Point;
   TP_dist = takeProfitPoints * _Point;
   return(INIT_SUCCEEDED);
}

void OnDeinit(const int reason)
{
}

void OnTick()
{
   datetime now = TimeCurrent();

   // reset daily state
   int dk = DayKey(now);
   if(dk != lastDayKey)
   {
      lastDayKey = dk;
      ordersPlacedToday = false;
   }

   londonOpenTime  = TodayAtHour(now, londonOpenHour);
   londonCloseTime = TodayAtHour(now, londonCloseHour);

   // expire logic: after London close, ensure pending orders are gone
   if(now >= londonCloseTime)
   {
      CancelPending();
      return;
   }

   // Only place orders once per day, at/after London open
   if(!ordersPlacedToday && now >= londonOpenTime)
   {
      // if a position is already open, do not place new orders
      if(PositionSelect(_Symbol)) { ordersPlacedToday = true; return; }

      double asiaHigh, asiaLow;
      if(!ComputeAsianRange(now, asiaHigh, asiaLow))
      {
         ordersPlacedToday = true; // avoid spamming attempts
         return;
      }

      double buffer = bufferPoints * _Point;

      double buyLevel  = asiaHigh + buffer;
      double sellLevel = asiaLow  - buffer;

      double lots = GetPositionSize(SL_dist);

      // Set expiration at London close
      datetime expiry = londonCloseTime;

      // Place both stop orders
      trade.BuyStop(lots, buyLevel, _Symbol,
                    buyLevel - SL_dist, buyLevel + TP_dist,
                    ORDER_TIME_SPECIFIED, expiry);

      trade.SellStop(lots, sellLevel, _Symbol,
                     sellLevel + SL_dist, sellLevel - TP_dist,
                     ORDER_TIME_SPECIFIED, expiry);

      ordersPlacedToday = true;
      return;
   }
}
