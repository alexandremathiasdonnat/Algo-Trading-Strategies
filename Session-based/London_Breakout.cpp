#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <vector>

enum class PosState { FLAT=0, LONG=1, SHORT=-1 };

struct Trade {
    int day = -1;
    int entry_idx = -1;
    int exit_idx  = -1;
    PosState side = PosState::FLAT;
    double entry_px = 0.0;
    double exit_px  = 0.0;
    std::string reason;
    double pnl = 0.0; // signed (in price units)
};

struct Bar {
    double open=0, high=0, low=0, close=0;
};

int main() {
    // --- Intraday simulation setup
    const int days = 120;
    const int bars_per_day = 24 * 12;          // 5-min bars
    const int T = days * bars_per_day;

    // Session times (in "day minutes", consistent with 5-min bars)
    // Asia: 00:00 -> 08:00, London: 09:00 -> 18:00 (same convention as README)
    const int asia_start_bar   = 0 * 12;       // 00:00
    const int asia_end_bar     = 8 * 12;       // 08:00 (exclusive)
    const int london_open_bar  = 9 * 12;       // 09:00
    const int london_close_bar = 18 * 12;      // 18:00 (exclusive)

    // Breakout buffer (in price units, synthetic)
    const double buffer = 0.00;

    // Risk parameters (percent of entry price)
    const bool   useSLTP = true;
    const double alphaSL = 0.006;  // 0.6%
    const double alphaTP = 0.012;  // 1.2%

    // --- Synthetic price process with time-of-day volatility pattern
    const double S0 = 100.0;
    const uint32_t seed = 7;
    std::mt19937 rng(seed);
    std::normal_distribution<double> N(0.0, 1.0);

    // vol schedule: low in Asia, higher around London
    auto vol_for_bar = [&](int bar_in_day) {
        if (bar_in_day >= asia_start_bar && bar_in_day < asia_end_bar) return 0.0006;
        if (bar_in_day >= london_open_bar && bar_in_day < london_open_bar + 6) return 0.0022; // burst
        if (bar_in_day >= london_open_bar && bar_in_day < london_close_bar) return 0.0012;
        return 0.0008;
    };

    std::vector<Bar> bars(T);
    double last = S0;

    for (int t = 0; t < T; ++t) {
        int bar_in_day = t % bars_per_day;

        double sigma = vol_for_bar(bar_in_day);
        double z1 = N(rng);
        double z2 = N(rng);
        double z3 = N(rng);

        // build a simple OHLC around a random walk close
        double open = last;
        double close = open * std::exp(-0.5*sigma*sigma + sigma*z1);

        // intrabar high/low approximations
        double h = std::max(open, close) * std::exp(std::fabs(sigma*z2));
        double l = std::min(open, close) / std::exp(std::fabs(sigma*z3));

        bars[t] = {open, h, l, close};
        last = close;
    }

    // --- Strategy state
    PosState pos = PosState::FLAT;
    double entry = 0.0;
    int entry_idx = -1;

    bool pending_buy = false, pending_sell = false;
    double buy_level = 0.0, sell_level = 0.0;

    std::vector<Trade> trades;
    trades.reserve(256);

    auto open_pos = [&](int day, int idx, PosState side, double px) {
        pos = side;
        entry = px;
        entry_idx = idx;
        // once one side triggers, cancel the other
        pending_buy = pending_sell = false;
    };

    auto close_pos = [&](int day, int idx, double px, const std::string& reason) {
        Trade tr;
        tr.day = day;
        tr.entry_idx = entry_idx;
        tr.exit_idx  = idx;
        tr.side      = pos;
        tr.entry_px  = entry;
        tr.exit_px   = px;
        tr.reason    = reason;

        int s = static_cast<int>(pos);
        tr.pnl = (px - entry) * s;

        trades.push_back(tr);

        pos = PosState::FLAT;
        entry = 0.0;
        entry_idx = -1;
    };

    // --- Main loop day by day
    for (int d = 0; d < days; ++d) {
        int day_start = d * bars_per_day;

        // 1) compute Asian range from bars [asia_start, asia_end)
        double asiaHigh = -1e100;
        double asiaLow  =  1e100;
        for (int i = day_start + asia_start_bar; i < day_start + asia_end_bar; ++i) {
            asiaHigh = std::max(asiaHigh, bars[i].high);
            asiaLow  = std::min(asiaLow,  bars[i].low);
        }

        // 2) at London open, place two stop orders (if flat)
        if (pos == PosState::FLAT) {
            pending_buy = true;
            pending_sell = true;
            buy_level  = asiaHigh + buffer;
            sell_level = asiaLow  - buffer;
        }

        // 3) trade during London session only
        for (int bi = london_open_bar; bi < london_close_bar; ++bi) {
            int t = day_start + bi;
            const auto& b = bars[t];

            // --- If in position, check SL/TP first (evaluated independently of signals)
            if (useSLTP && pos != PosState::FLAT) {
                int s = static_cast<int>(pos);
                double SL = entry * (1.0 - s * alphaSL);
                double TP = entry * (1.0 + s * alphaTP);

                bool hitSL = (s == +1) ? (b.low  <= SL) : (b.high >= SL);
                bool hitTP = (s == +1) ? (b.high >= TP) : (b.low  <= TP);

                if (hitSL) { close_pos(d, t, SL, "SL"); break; }
                if (hitTP) { close_pos(d, t, TP, "TP"); break; }
            }

            // --- If flat, check pending stops (breakout)
            if (pos == PosState::FLAT) {
                // approximate priority if both hit same bar:
                // choose the level closer to open (very minor detail; still deterministic)
                bool hitBuy  = pending_buy  && (b.high >= buy_level);
                bool hitSell = pending_sell && (b.low  <= sell_level);

                if (hitBuy && hitSell) {
                    double distBuy  = std::fabs(b.open - buy_level);
                    double distSell = std::fabs(b.open - sell_level);
                    if (distBuy <= distSell) open_pos(d, t, PosState::LONG,  buy_level);
                    else                     open_pos(d, t, PosState::SHORT, sell_level);
                    continue;
                }

                if (hitBuy)  { open_pos(d, t, PosState::LONG,  buy_level);  continue; }
                if (hitSell) { open_pos(d, t, PosState::SHORT, sell_level); continue; }
            }
        }

        // 4) at London close: expire pending orders; optionally flat by session end
        pending_buy = pending_sell = false;

        // optional: force exit at session end if still in position
        int end_t = day_start + (london_close_bar - 1);
        if (pos != PosState::FLAT) {
            close_pos(d, end_t, bars[end_t].close, "SessionClose");
        }
    }

    // --- Reporting
    double total_pnl = 0.0;
    int wins = 0, losses = 0;
    for (auto& tr : trades) {
        total_pnl += tr.pnl;
        if (tr.pnl >= 0) wins++; else losses++;
    }

    std::cout << "London Breakout (standalone C++)\n";
    std::cout << "Days: " << days
              << " | Trades: " << trades.size()
              << " | Wins: " << wins
              << " | Losses: " << losses
              << " | Total PnL (price units): " << std::fixed << std::setprecision(4) << total_pnl
              << "\n";

    if (!trades.empty()) {
        std::cout << "\nLast 5 trades:\n";
        int n = (int)trades.size();
        int start = std::max(0, n - 5);
        for (int i = start; i < n; ++i) {
            const auto& tr = trades[i];
            std::cout << " - day " << tr.day
                      << " [" << tr.entry_idx << " -> " << tr.exit_idx << "] "
                      << (tr.side == PosState::LONG ? "LONG" : "SHORT")
                      << " entry=" << tr.entry_px
                      << " exit=" << tr.exit_px
                      << " pnl=" << tr.pnl
                      << " reason=" << tr.reason
                      << "\n";
        }
    }

    return 0;
}
