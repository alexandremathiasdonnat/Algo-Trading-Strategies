#include <cmath>
#include <iostream>
#include <vector>
#include <numeric>
#include <random>

enum class Signal { SHORT = -1, FLAT = 0, LONG = 1 };

double sign(double x) {
    if (x > 0) return 1.0;
    if (x < 0) return -1.0;
    return 0.0;
}

Signal trend_expert(double ret_prev) {
    return static_cast<Signal>(sign(ret_prev));
}

Signal mean_reversion_expert(double ret_prev) {
    return static_cast<Signal>(-sign(ret_prev));
}

Signal noise_expert(std::mt19937& rng) {
    std::uniform_int_distribution<int> U(-1, 1);
    return static_cast<Signal>(U(rng));
}

int main() {
    const int T = 3000;
    const double eta = 0.5;

    std::mt19937 rng(123);
    std::normal_distribution<double> N(0.0, 0.01);

    std::vector<double> weights = {1.0/3, 1.0/3, 1.0/3};
    std::vector<double> returns(T);

    for (int t = 1; t < T; ++t) {
        double r = N(rng);
        returns[t] = r;

        Signal e1 = trend_expert(returns[t-1]);
        Signal e2 = mean_reversion_expert(returns[t-1]);
        Signal e3 = noise_expert(rng);

        std::vector<Signal> experts = {e1, e2, e3};

        double agg = 0.0;
        for (size_t i = 0; i < experts.size(); ++i)
            agg += weights[i] * static_cast<int>(experts[i]);

        Signal decision = static_cast<Signal>(sign(agg));

        double realized = static_cast<int>(decision) * r;

        for (size_t i = 0; i < weights.size(); ++i) {
            weights[i] *= std::exp(eta * static_cast<int>(experts[i]) * r);
        }

        double norm = std::accumulate(weights.begin(), weights.end(), 0.0);
        for (auto& w : weights) w /= norm;

        if (t % 500 == 0) {
            std::cout << "t=" << t
                      << " | w_trend=" << weights[0]
                      << " w_mr=" << weights[1]
                      << " w_noise=" << weights[2]
                      << "\n";
        }
    }

    return 0;
}
