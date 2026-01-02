#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <vector>
#include <algorithm>

enum class PosState { FLAT=0, LONG=1, SHORT=-1 };

struct Bar {
    double open=0, high=0, low=0, close=0;
};

struct Trade {
    int entry_idx=-1, exit_idx=-1;
    PosState side=PosState::FLAT;
    double entry_px=0, exit_px=0;
    std::string reason;
    double pnl=0.0; // signed
};

static double true_range(const Bar& b, double prev_close) {
    double tr1 = b.high - b.low;
    double tr2 = std::fabs(b.high - prev_close);
    double tr3 = std::fabs(b.low  - prev_close);
    return std::max({tr1, tr2, tr3});
}

static double sma(const std::vector<double>& x, int end_incl, int win) {
    // SMA over x[end_incl-win+1 .. end_incl]
    double s=0.0;
    for(int i=end_incl-win+1;i<=end_incl;i++) s+=x[i];
    return s / win;
}

int main() {
    // --- Synthetic OHLC generation with volatility regimes
    const int T = 5000;
    const double S0 = 100.0;
    const uint32_t seed = 123;
    std::mt19937 rng(seed);
    std::normal_distribution<double> N(0.0, 1.0);

    auto sigma_regime = [&](int t){
        // alternating regimes
        if (t < 1500) return 0.0008;      // low vol
        if (t < 2500) return 0.0020;      // high vol expansion
        if (t < 3500) return 0.0009;      // back to low
        return 0.0018;                   // high again
    };

    std::vector<Bar> bars(T);
    double last = S0;
    for(int t=0;t<T;t++){
        double sigma = sigma_regime(t);
        double z1=N(rng), z2=N(rng), z3=N(rng);

        double open = last;
        double close = open * std::exp(-0.5*sigma*sigma + sigma*z1);

        double high = std::max(open, close) * std::exp(std::fabs(sigma*z2));
        double low  = std::min(open, close) / std::exp(std::fabs(sigma*z3));

        bars[t] = {open, high, low, close};
        last = close;
    }

    // --- ATR parameters
    const int atrFast = 14;
    const int atrSlow = 50;
    const double mult = 1.50;

    // --- Breakout definition: Close(t) vs High/Low(t-1)
    // (we use closed-bar logic; decision at t uses bars[t-1] and indicators up to t-1)
    // --- Risk parameters (percent of entry)
    const bool useSLTP = true;
    const double alphaSL = 0.008; // 0.8%
    const double alphaTP = 0.016; // 1.6%

    // --- Precompute TR and ATR series
    std::vector<double> tr(T, 0.0);
    for(int t=1;t<T;t++){
        tr[t] = true_range(bars[t], bars[t-1].close);
    }

    std::vector<double> atrF(T, 0.0), atrS(T, 0.0);
    for(int t=0;t<T;t++){
        if(t >= atrFast) atrF[t] = sma(tr, t, atrFast);
        if(t >= atrSlow) atrS[t] = sma(tr, t, atrSlow);
    }

    // --- Strategy state
    PosState pos = PosState::FLAT;
    double entry=0.0;
    int entry_idx=-1;

    std::vector<Trade> trades;
    trades.reserve(256);

    auto open_pos = [&](int idx, PosState side, double px){
        pos = side;
        entry = px;
        entry_idx = idx;
    };

    auto close_pos = [&](int idx, double px, const std::string& reason){
        Trade trd;
        trd.entry_idx = entry_idx;
        trd.exit_idx = idx;
        trd.side = pos;
        trd.entry_px = entry;
        trd.exit_px = px;
        trd.reason = reason;
        int s = static_cast<int>(pos);
        trd.pnl = (px - entry) * s;
        trades.push_back(trd);

        pos = PosState::FLAT;
        entry=0.0;
        entry_idx=-1;
    };

    // --- Main loop (closed bars only)
    for(int t = atrSlow + 2; t < T; ++t) {
        int sig_idx = t-1; // last closed bar

        // --- If in position: evaluate SL/TP first
        if(useSLTP && pos != PosState::FLAT) {
            int s = static_cast<int>(pos);
            double SL = entry * (1.0 - s * alphaSL);
            double TP = entry * (1.0 + s * alphaTP);

            const auto& b = bars[sig_idx];

            bool hitSL = (s == +1) ? (b.low  <= SL) : (b.high >= SL);
            bool hitTP = (s == +1) ? (b.high >= TP) : (b.low  <= TP);

            if(hitSL) { close_pos(sig_idx, SL, "SL"); continue; }
            if(hitTP) { close_pos(sig_idx, TP, "TP"); continue; }
        }

        // --- Signals on closed bar sig_idx
        bool expansion = (atrF[sig_idx] > mult * atrS[sig_idx]);

        double C = bars[sig_idx].close;
        double Hprev = bars[sig_idx-1].high;
        double Lprev = bars[sig_idx-1].low;

        bool breakoutUp   = (C > Hprev);
        bool breakoutDown = (C < Lprev);

        if(pos == PosState::FLAT && expansion) {
            // Keep deterministic priority: if both true (rare), choose based on distance to close
            if(breakoutUp && breakoutDown) {
                double dUp = std::fabs(C - Hprev);
                double dDn = std::fabs(C - Lprev);
                if(dUp <= dDn) open_pos(sig_idx, PosState::LONG,  C);
                else           open_pos(sig_idx, PosState::SHORT, C);
                continue;
            }
            if(breakoutUp)   { open_pos(sig_idx, PosState::LONG,  C); continue; }
            if(breakoutDown) { open_pos(sig_idx, PosState::SHORT, C); continue; }
        }
    }

    // Close any open position at end
    if(pos != PosState::FLAT) {
        close_pos(T-1, bars[T-1].close, "EOD");
    }

    // --- Reporting
    double total_pnl = 0.0;
    int wins=0, losses=0;
    for(const auto& trd: trades){
        total_pnl += trd.pnl;
        if(trd.pnl >= 0) wins++; else losses++;
    }

    std::cout << "ATR Expansion Breakout (standalone C++)\n";
    std::cout << "Trades: " << trades.size()
              << " | Wins: " << wins
              << " | Losses: " << losses
              << " | Total PnL (price units): " << std::fixed << std::setprecision(4) << total_pnl
              << "\n";

    if(!trades.empty()){
        std::cout << "\nLast 5 trades:\n";
        int n = (int)trades.size();
        for(int i = std::max(0, n-5); i < n; ++i){
            const auto& trd = trades[i];
            std::cout << " - [" << trd.entry_idx << " -> " << trd.exit_idx << "] "
                      << (trd.side == PosState::LONG ? "LONG" : "SHORT")
                      << " entry=" << trd.entry_px
                      << " exit=" << trd.exit_px
                      << " pnl=" << trd.pnl
                      << " reason=" << trd.reason
                      << "\n";
        }
    }

    return 0;
}
