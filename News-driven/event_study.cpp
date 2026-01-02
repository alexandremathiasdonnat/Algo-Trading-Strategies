#include <cmath>
#include <iostream>
#include <random>
#include <vector>
#include <string>

enum class Regime { RISK_ON, RISK_OFF };
enum class EventType { NONE, MACRO, CENTRAL_BANK };

struct MarketPoint {
    int t;
    double price;
    double ret;
    Regime regime;
    EventType event_type;
    double surprise;     // only meaningful if event_type != NONE
};

static std::string to_string(Regime r){
    return (r == Regime::RISK_ON) ? "RISK_ON" : "RISK_OFF";
}
static std::string to_string(EventType e){
    if(e == EventType::MACRO) return "MACRO";
    if(e == EventType::CENTRAL_BANK) return "CENTRAL_BANK";
    return "NONE";
}

int main(){
    const int T = 4000;

    // Baseline dynamics
    const double sigma_base = 0.005;

    // Event dynamics
    const double sigma_event_macro = 0.020;
    const double sigma_event_cb    = 0.030;

    // Surprise-driven jump magnitude
    const double jump_scale_macro = 0.040;
    const double jump_scale_cb    = 0.060;

    // Regime switching
    const double p_switch = 0.002; // per step

    // Event calendar
    // Macro events every 400 steps, central bank events every 800 steps
    std::vector<int> macro_events;
    std::vector<int> cb_events;
    for(int t = 400; t < T; t += 400) macro_events.push_back(t);
    for(int t = 800; t < T; t += 800) cb_events.push_back(t);

    std::mt19937 rng(42);
    std::normal_distribution<double> N(0.0, 1.0);
    std::uniform_real_distribution<double> U(0.0, 1.0);

    double price = 100.0;
    Regime regime = Regime::RISK_ON;

    for(int t = 0; t < T; ++t){
        // Regime switching (Markov-ish)
        if(U(rng) < p_switch){
            regime = (regime == Regime::RISK_ON) ? Regime::RISK_OFF : Regime::RISK_ON;
        }

        // Determine event type at t
        EventType et = EventType::NONE;
        for(int e: cb_events){
            if(t == e){ et = EventType::CENTRAL_BANK; break; }
        }
        if(et == EventType::NONE){
            for(int e: macro_events){
                if(t == e){ et = EventType::MACRO; break; }
            }
        }

        // Surprise only at event timestamps
        double surprise = 0.0;
        if(et != EventType::NONE){
            surprise = N(rng); // N(0,1) surprise proxy
        }

        // Calendar effect (toy): periodic flow day every 1000 steps
        // Demonstrates conditioning hooks.
        double calendar_drift = ((t % 1000) > 950) ? 0.0005 : 0.0;

        // Regime drift (toy): risk-on tends to positive drift, risk-off negative
        double regime_drift = (regime == Regime::RISK_ON) ? 0.0002 : -0.0002;

        // Volatility depends on event type
        double sigma = sigma_base;
        if(et == EventType::MACRO) sigma = sigma_event_macro;
        if(et == EventType::CENTRAL_BANK) sigma = sigma_event_cb;

        // Base return
        double ret = regime_drift + calendar_drift + sigma * N(rng);

        // Surprise-driven jump on event timestamps
        if(et == EventType::MACRO){
            ret += jump_scale_macro * surprise;
        } else if(et == EventType::CENTRAL_BANK){
            ret += jump_scale_cb * surprise;
        }

        price *= std::exp(ret);

        MarketPoint mp{t, price, ret, regime, et, surprise};

        // Output format: t price ret regime event_type surprise
        std::cout << mp.t << " "
                  << mp.price << " "
                  << mp.ret << " "
                  << to_string(mp.regime) << " "
                  << to_string(mp.event_type) << " "
                  << mp.surprise
                  << "\n";
    }

    return 0;
}
