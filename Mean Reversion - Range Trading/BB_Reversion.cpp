#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <vector>

enum class PosState { FLAT=0, LONG=1, SHORT=-1 };

struct Trade {
    int entry_idx = -1;
    int exit_idx  = -1;
    PosState side = PosState::FLAT;
    double entry_px = 0.0;
    double exit_px  = 0.0;
    std::string reason;
    double pnl = 0.0; // signed (in price units)
};

static double mean(const std::vector<double>& x, int start, int end_excl) {
    double s = 0.0;
    for (int i = start; i < end_excl; ++i) s += x[i];
    return s / (end_excl - start);
}

static double stdev(const std::vector<double>& x, int start, int end_excl, double m) {
    double ss = 0.0;
    for (int i = start; i < end_excl; ++i) {
        double d = x[i] - m;
        ss += d * d;
    }
    return std::sqrt(ss / (end_excl - start));
}

int main() {
    // --- Synthetic price generation (GBM-like random walk)
    const int    T = 4000;        // number of bars
    const double S0 = 100.0;
    const double mu = 0.00;       // drift per step
    const double sigma = 0.01;    // vol per step
    const uint32_t seed = 42;

    std::mt19937 rng(seed);
    std::normal_distribution<double> N(0.0, 1.0);

    std::vector<double> close(T);
    close[0] = S0;
    for (int t = 1; t < T; ++t) {
        double z = N(rng);
        // lognormal step
        close[t] = close[t-1] * std::exp((mu - 0.5*sigma*sigma) + sigma*z);
    }

    // --- Bollinger parameters
    const int    N_bb = 20;
    const double k    = 2.0;

    // --- Risk parameters (percent of entry price)
    const bool   useSLTP = true;
    const double alphaSL = 0.01; // 1%
    const double alphaTP = 0.01; // 1%

    // --- Strategy state
    PosState pos = PosState::FLAT;
    double entry = 0.0;
    int entry_idx = -1;

    std::vector<Trade> trades;
    trades.reserve(256);

    auto open_trade = [&](int idx, PosState side, double px) {
        pos = side;
        entry = px;
        entry_idx = idx;
    };

    auto close_trade = [&](int idx, double px, const std::string& reason) {
        Trade tr;
        tr.entry_idx = entry_idx;
        tr.exit_idx  = idx;
        tr.side      = pos;
        tr.entry_px  = entry;
        tr.exit_px   = px;
        tr.reason    = reason;

        int s = static_cast<int>(pos); // +1 long, -1 short
        tr.pnl = (px - entry) * s;

        trades.push_back(tr);

        pos = PosState::FLAT;
        entry = 0.0;
        entry_idx = -1;
    };

    // --- Main loop (closed-bar logic)
    // We compute BB at time t using window [t-N_bb, t) and make decisions based on close[t-1]
    // to avoid lookahead (signal uses last closed bar).
    for (int t = N_bb + 1; t < T; ++t) {
        // compute BB using past N_bb closes ending at t-1 (exclusive of t)
        int start = t - N_bb;
        int end   = t; // [start, end)
        double m = mean(close, start, end);
        double sd = stdev(close, start, end, m);

        double bb_mid = m;
        double bb_up  = m + k * sd;
        double bb_lo  = m - k * sd;

        // last closed bar price
        double P = close[t-1];

        // --- Risk management: recompute SL/TP from entry at each step
        if (useSLTP && pos != PosState::FLAT) {
            int s = static_cast<int>(pos); // +1 long, -1 short

            // SL = P0 * (1 - s * alphaSL)
            // TP = P0 * (1 + s * alphaTP)
            double SL = entry * (1.0 - s * alphaSL);
            double TP = entry * (1.0 + s * alphaTP);

            bool hitSL = (s == +1) ? (P <= SL) : (P >= SL);
            bool hitTP = (s == +1) ? (P >= TP) : (P <= TP);

            if (hitSL) { close_trade(t-1, SL, "SL"); continue; }
            if (hitTP) { close_trade(t-1, TP, "TP"); continue; }
        }

        // --- Entry logic (1 position max): fade BB extremes
        if (pos == PosState::FLAT) {
            // Short setup: close above upper band
            if (P > bb_up) {
                open_trade(t-1, PosState::SHORT, P);
                continue;
            }
            // Long setup: close below lower band
            if (P < bb_lo) {
                open_trade(t-1, PosState::LONG, P);
                continue;
            }
        }

        // We could add a signal-based exit (e.g., close at mid band),
        // but we keep it minimal: exits are driven by SL/TP only in this baseline.
    }

    // If still open at the end, close at last price
    if (pos != PosState::FLAT) {
        close_trade(T-1, close[T-1], "EOD");
    }

    // --- Reporting
    double total_pnl = 0.0;
    int wins = 0, losses = 0;
    for (auto& tr : trades) {
        total_pnl += tr.pnl;
        if (tr.pnl >= 0) wins++; else losses++;
    }

    std::cout << "Bollinger Bands Reversion (standalone C++)\n";
    std::cout << "Trades: " << trades.size()
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
            std::cout << " - [" << tr.entry_idx << " -> " << tr.exit_idx << "] "
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
