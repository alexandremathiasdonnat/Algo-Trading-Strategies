#include <cmath>
#include <iostream>
#include <vector>
#include <numeric>
#include <algorithm>
#include <iomanip>
#include <random>

enum class PosState { FLAT, LONG_SPREAD, SHORT_SPREAD };

struct PairPoint { double x, y; };

struct Trade {
    int entry=-1, exit=-1;
    PosState side=PosState::FLAT;
    double entry_spread=0.0, exit_spread=0.0;
    double pnl=0.0;
    std::string reason;
};

static double mean(const std::vector<double>& v, int start, int end) {
    double s = 0.0;
    for(int i=start;i<=end;i++) s += v[i];
    return s / (end - start + 1);
}

static double stdev(const std::vector<double>& v, int start, int end, double mu) {
    double s2 = 0.0;
    for(int i=start;i<=end;i++){
        double d = v[i] - mu;
        s2 += d*d;
    }
    double var = s2 / (end - start + 1);
    return std::sqrt(std::max(var, 1e-12));
}

// Rolling OLS: y ≈ a + b x
static void rolling_ols_beta_alpha(
    const std::vector<PairPoint>& data,
    int t_end,
    int L,
    double& a,
    double& b
){
    int start = t_end - L + 1;
    double mx=0.0, my=0.0;
    for(int i=start;i<=t_end;i++){
        mx += data[i].x;
        my += data[i].y;
    }
    mx /= L; my /= L;

    double num=0.0, den=0.0;
    for(int i=start;i<=t_end;i++){
        double dx = data[i].x - mx;
        double dy = data[i].y - my;
        num += dx * dy;
        den += dx * dx;
    }
    b = (den > 1e-12) ? (num / den) : 0.0;
    a = my - b * mx;
}

int main(){
    // --- Build a synthetic cointegrated pair in-code (no external dependency)
    const int T = 4000;
    const double true_beta = 1.25;

    std::mt19937 rng(42);
    std::normal_distribution<double> N(0.0, 1.0);

    std::vector<PairPoint> data;
    data.reserve(T);

    double x=100.0;
    for(int t=0;t<T;t++){
        x += 0.2 * N(rng);
        double sigma = (t < T/2) ? 0.5 : 1.5;
        double y = true_beta * x + sigma * N(rng);
        data.push_back({x,y});
    }

    // --- Strategy params
    const int L_beta = 200;     // rolling OLS window
    const int L_z    = 200;     // rolling zscore window on spread
    const double z_entry = 2.0;
    const double z_exit  = 0.5;
    const int max_hold   = 400;

    std::vector<double> spread(T, 0.0);

    PosState pos = PosState::FLAT;
    int entry_t=-1;
    double entry_sp=0.0;
    double pnl=0.0;

    std::vector<Trade> trades;
    trades.reserve(128);

    for(int t = std::max(L_beta, L_z); t < T; ++t){
        // estimate hedge ratio using only past data up to t-1 (closed-bar discipline)
        double a=0.0, b=0.0;
        rolling_ols_beta_alpha(data, t-1, L_beta, a, b);

        // current spread at time t-1 (signal evaluated on closed obs)
        int sig = t-1;
        spread[sig] = data[sig].y - (a + b * data[sig].x);

        // compute z-score using spread history up to sig
        int z_start = sig - L_z + 1;
        double mu = mean(spread, z_start, sig);
        double sd = stdev(spread, z_start, sig, mu);
        double z  = (spread[sig] - mu) / sd;

        // PnL accrual from t-1 to t on hedged portfolio if in position
        if(pos != PosState::FLAT){
            // portfolio weights consistent with spread: S = y - (a + b x)
            // Long spread: +1*y and -b*x ; Short spread: -1*y and +b*x
            int s = (pos == PosState::LONG_SPREAD) ? +1 : -1;
            double dy = data[t].y - data[t-1].y;
            double dx = data[t].x - data[t-1].x;
            pnl += s * (dy - b * dx);
        }

        // Exit logic first
        if(pos != PosState::FLAT){
            bool exit_cond = (std::fabs(z) < z_exit);
            bool time_stop = (t - entry_t >= max_hold);

            if(exit_cond || time_stop){
                Trade tr;
                tr.entry = entry_t;
                tr.exit  = t;
                tr.side  = pos;
                tr.entry_spread = entry_sp;
                tr.exit_spread  = spread[sig];
                tr.pnl = pnl;
                tr.reason = exit_cond ? "Z_EXIT" : "TIME_STOP";
                trades.push_back(tr);

                pos = PosState::FLAT;
                entry_t=-1;
                entry_sp=0.0;
                pnl=0.0;
            }
            continue;
        }

        // Entry logic
        if(z > z_entry){
            pos = PosState::SHORT_SPREAD;
            entry_t = t;
            entry_sp = spread[sig];
            pnl = 0.0;
        } else if(z < -z_entry){
            pos = PosState::LONG_SPREAD;
            entry_t = t;
            entry_sp = spread[sig];
            pnl = 0.0;
        }
    }

    // report
    double total = 0.0;
    int wins=0, losses=0;
    for(const auto& tr: trades){
        total += tr.pnl;
        if(tr.pnl >= 0) wins++; else losses++;
    }

    std::cout << "Pairs Trading (rolling OLS + z-score) - standalone C++\n";
    std::cout << "Trades: " << trades.size()
              << " | Wins: " << wins
              << " | Losses: " << losses
              << " | Total PnL (spread units): " << std::fixed << std::setprecision(4) << total
              << "\n";

    if(!trades.empty()){
        std::cout << "\nLast 5 trades:\n";
        int n=(int)trades.size();
        for(int i=std::max(0,n-5); i<n; ++i){
            const auto& tr = trades[i];
            std::cout << " - [" << tr.entry << " -> " << tr.exit << "] "
                      << (tr.side==PosState::LONG_SPREAD ? "LONG_SPREAD" : "SHORT_SPREAD")
                      << " pnl=" << tr.pnl
                      << " reason=" << tr.reason
                      << "\n";
        }
    }

    return 0;
}
