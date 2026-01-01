#include <iostream>
#include <vector>
#include <random>
#include <cmath>
#include <iomanip>
#include <string>

struct Trade {
    int side; // +1 long, -1 short
    int entry_idx, exit_idx;
    double entry, exit, pnl;
};

static double sma(const std::vector<double>& x, int end_idx, int window) {
    double s = 0.0;
    for (int i = end_idx - window + 1; i <= end_idx; ++i) s += x[i];
    return s / window;
}

// Generate synthetic close prices (GBM-like)
static std::vector<double> generate_prices(int n, double s0, double mu, double sigma, unsigned seed=42) {
    std::mt19937 rng(seed);
    std::normal_distribution<double> z(0.0, 1.0);

    std::vector<double> close(n);
    close[0] = s0;

    // dt = 1 per step
    for (int t = 1; t < n; ++t) {
        double eps = z(rng);
        double log_ret = (mu - 0.5 * sigma * sigma) + sigma * eps;
        close[t] = close[t-1] * std::exp(log_ret);
    }
    return close;
}

int main() {
    // --- Parameters (mirror MQ5 intent) ---
    const int fastN = 20;
    const int slowN = 50;

    const bool useSLTP = true;
    const double stopLossPct  = 0.01; // 1%
    const double takeProfitPct= 0.02; // 2%

    // Synthetic data config (internal data, no files)
    const int N = 2000;
    const double S0 = 100.0;
    const double mu = 0.0002;   // drift per step
    const double sigma = 0.01;  // vol per step
    const unsigned seed = 7;

    if (!(0 < fastN && fastN < slowN)) {
        std::cerr << "Invalid MA windows: need 0 < fast < slow\n";
        return 1;
    }
    if (N < slowN + 3) {
        std::cerr << "Not enough points.\n";
        return 1;
    }

    std::vector<double> close = generate_prices(N, S0, mu, sigma, seed);

    int pos = 0; // 0 flat, +1 long, -1 short
    double entry = 0.0;
    int entry_idx = -1;

    std::vector<Trade> trades;
    trades.reserve(200);

    auto close_pos = [&](int i, double exit_price) {
        Trade t;
        t.side = pos;
        t.entry_idx = entry_idx;
        t.exit_idx = i;
        t.entry = entry;
        t.exit = exit_price;
        t.pnl = (exit_price - entry) * (double)pos;
        trades.push_back(t);

        pos = 0;
        entry = 0.0;
        entry_idx = -1;
    };

    for (int i = slowN + 2; i < N; ++i) {
        // Crossover on CLOSED points: compare (i-2) and (i-1)
        int a = i - 2;
        int b = i - 1;

        double fast_a = sma(close, a, fastN);
        double slow_a = sma(close, a, slowN);
        double fast_b = sma(close, b, fastN);
        double slow_b = sma(close, b, slowN);

        bool bullishCross = (fast_a <= slow_a) && (fast_b > slow_b);
        bool bearishCross = (fast_a >= slow_a) && (fast_b < slow_b);

        // Risk check using close as proxy (demo purpose)
        if (pos != 0 && useSLTP) {
            double sl = entry * (1.0 - stopLossPct * (double)pos);
            double tp = entry * (1.0 + takeProfitPct * (double)pos);

            double px = close[i];

            // Conservative: if both theoretically crossed, take SL first.
            bool hitSL = (pos == +1) ? (px <= sl) : (px >= sl);
            bool hitTP = (pos == +1) ? (px >= tp) : (px <= tp);

            if (hitSL) { close_pos(i, sl); continue; }
            if (hitTP) { close_pos(i, tp); continue; }
        }

        // Flip logic
        if (bullishCross) {
            if (pos == -1) close_pos(i, close[i]);
            if (pos == 0) { pos = +1; entry = close[i]; entry_idx = i; }
        } else if (bearishCross) {
            if (pos == +1) close_pos(i, close[i]);
            if (pos == 0) { pos = -1; entry = close[i]; entry_idx = i; }
        }
    }

    if (pos != 0) close_pos(N-1, close.back());

    // Summary
    double totalPnL = 0.0;
    int wins = 0;
    double maxDD = 0.0;
    double equity = 0.0;
    double peak = 0.0;

    for (auto &t : trades) {
        totalPnL += t.pnl;
        equity += t.pnl;
        peak = std::max(peak, equity);
        maxDD = std::max(maxDD, peak - equity);
        if (t.pnl > 0) wins++;
    }

    std::cout << std::fixed << std::setprecision(4);
    std::cout << "Standalone C++ MA Crossover (synthetic data)\n";
    std::cout << "N=" << N << " fast=" << fastN << " slow=" << slowN << "\n";
    std::cout << "Trades: " << trades.size() << "\n";
    if (!trades.empty()) {
        std::cout << "Win rate: " << (100.0 * wins / (double)trades.size()) << "%\n";
    }
    std::cout << "Total PnL (price units): " << totalPnL << "\n";
    std::cout << "Max Drawdown (PnL units): " << maxDD << "\n";

    return 0;
}
