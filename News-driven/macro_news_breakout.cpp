#include <cmath>
#include <iostream>
#include <random>
#include <vector>
#include <iomanip>
#include <string>

enum class Regime { RISK_ON, RISK_OFF };
enum class EventType { NONE, MACRO, CENTRAL_BANK };
enum class PosState { FLAT, LONG, SHORT };

struct Tick {
    int t;
    double price;
    double ret;
    Regime regime;
    EventType event_type;
    double surprise;
};

static std::string to_string(Regime r){
    return (r == Regime::RISK_ON) ? "RISK_ON" : "RISK_OFF";
}
static std::string to_string(EventType e){
    if(e == EventType::MACRO) return "MACRO";
    if(e == EventType::CENTRAL_BANK) return "CENTRAL_BANK";
    return "NONE";
}

// Build the same synthetic world internally (standalone, no external files)
static std::vector<Tick> generate_world(int T, unsigned seed = 7){
    const double sigma_base = 0.005;
    const double sigma_event_macro = 0.020;
    const double sigma_event_cb    = 0.030;

    const double jump_scale_macro = 0.040;
    const double jump_scale_cb    = 0.060;

    const double p_switch = 0.002;

    std::vector<int> macro_events;
    std::vector<int> cb_events;
    for(int t = 400; t < T; t += 400) macro_events.push_back(t);
    for(int t = 800; t < T; t += 800) cb_events.push_back(t);

    std::mt19937 rng(seed);
    std::normal_distribution<double> N(0.0, 1.0);
    std::uniform_real_distribution<double> U(0.0, 1.0);

    double price = 100.0;
    Regime regime = Regime::RISK_ON;

    std::vector<Tick> out;
    out.reserve(T);

    for(int t=0;t<T;++t){
        if(U(rng) < p_switch){
            regime = (regime == Regime::RISK_ON) ? Regime::RISK_OFF : Regime::RISK_ON;
        }

        EventType et = EventType::NONE;
        for(int e: cb_events){ if(t == e){ et = EventType::CENTRAL_BANK; break; } }
        if(et == EventType::NONE){
            for(int e: macro_events){ if(t == e){ et = EventType::MACRO; break; } }
        }

        double surprise = 0.0;
        if(et != EventType::NONE) surprise = N(rng);

        double calendar_drift = ((t % 1000) > 950) ? 0.0005 : 0.0;
        double regime_drift = (regime == Regime::RISK_ON) ? 0.0002 : -0.0002;

        double sigma = sigma_base;
        if(et == EventType::MACRO) sigma = sigma_event_macro;
        if(et == EventType::CENTRAL_BANK) sigma = sigma_event_cb;

        double ret = regime_drift + calendar_drift + sigma * N(rng);

        if(et == EventType::MACRO)        ret += jump_scale_macro * surprise;
        else if(et == EventType::CENTRAL_BANK) ret += jump_scale_cb * surprise;

        price *= std::exp(ret);

        out.push_back(Tick{t, price, ret, regime, et, surprise});
    }
    return out;
}

int main(){
    const int T = 5000;
    auto data = generate_world(T);

    // --- Strategy parameters
    const double k_surprise = 0.75;      // entry threshold on |surprise|
    const int hold_horizon = 30;         // fixed time exit (news effect decays)
    const double stop_pct = 0.020;       // stop-like bound (toy)
    const double take_pct = 0.030;       // take-like bound (toy)

    // Regime filter: scale exposure by regime
    const double size_risk_on  = 1.0;
    const double size_risk_off = 0.6;

    // Central bank day bias: bigger risk on CB events
    const double cb_size_multiplier = 1.3;

    // Calendar effects: periodic flow days get size bump
    // (here: last ~50 steps of each 1000-block)
    const double cal_size_multiplier = 1.2;

    // --- Trading state
    PosState pos = PosState::FLAT;
    int entry_t = -1;
    double entry_price = 0.0;
    double stop = 0.0;
    double take = 0.0;
    double size = 0.0;

    double pnl = 0.0;
    int trades_closed = 0;
    int trades_opened = 0;

    for(int t=1; t<T; ++t){
        const auto& cur = data[t];

        // If in position, update PnL mark-to-market on returns
        if(pos != PosState::FLAT){
            double dp = cur.price - data[t-1].price;
            if(pos == PosState::LONG) pnl += size * dp;
            else pnl += size * (-dp);
        }

        // Risk exits (stop/take) evaluated at current price
        if(pos != PosState::FLAT){
            if(pos == PosState::LONG){
                if(cur.price <= stop || cur.price >= take){
                    pos = PosState::FLAT;
                    trades_closed++;
                }
            } else {
                if(cur.price >= stop || cur.price <= take){
                    pos = PosState::FLAT;
                    trades_closed++;
                }
            }
        }

        // Time exit
        if(pos != PosState::FLAT && (t - entry_t >= hold_horizon)){
            pos = PosState::FLAT;
            trades_closed++;
        }

        // Entry only at event timestamps AND only if flat
        if(pos == PosState::FLAT && cur.event_type != EventType::NONE){
            if(std::fabs(cur.surprise) >= k_surprise){
                // Base sizing from regime
                double base_size = (cur.regime == Regime::RISK_ON) ? size_risk_on : size_risk_off;

                // Calendar sizing bump
                bool flow_day = ((cur.t % 1000) > 950);
                double cal_mult = flow_day ? cal_size_multiplier : 1.0;

                // CB sizing bump
                double cb_mult = (cur.event_type == EventType::CENTRAL_BANK) ? cb_size_multiplier : 1.0;

                size = base_size * cal_mult * cb_mult;

                // Direction = sign(surprise)  (news breakout abstraction)
                if(cur.surprise > 0){
                    pos = PosState::LONG;
                    entry_t = t;
                    entry_price = cur.price;
                    stop = entry_price * (1.0 - stop_pct);
                    take = entry_price * (1.0 + take_pct);
                } else {
                    pos = PosState::SHORT;
                    entry_t = t;
                    entry_price = cur.price;
                    stop = entry_price * (1.0 + stop_pct);
                    take = entry_price * (1.0 - take_pct);
                }

                trades_opened++;
            }
        }

        // periodic log
        if(t % 1000 == 0){
            std::cout << "t=" << t
                      << " pnl=" << std::fixed << std::setprecision(4) << pnl
                      << " opened=" << trades_opened
                      << " closed=" << trades_closed
                      << "\n";
        }
    }

    std::cout << "\nEvent/Macro/News-Driven: Macro Surprise Breakout (standalone C++)\n";
    std::cout << "Trades opened: " << trades_opened << "\n";
    std::cout << "Trades closed: " << trades_closed << "\n";
    std::cout << "Total PnL (synthetic units): " << std::fixed << std::setprecision(4) << pnl << "\n";

    return 0;
}
