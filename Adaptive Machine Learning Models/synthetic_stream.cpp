#include <cmath>
#include <iostream>
#include <random>
#include <vector>

enum class Regime { TREND, MEAN_REVERT, NOISE };

struct MarketPoint {
    double price;
    double ret;
    Regime regime;
};

std::vector<MarketPoint> generate_market(int T, unsigned seed = 42) {
    std::mt19937 rng(seed);
    std::normal_distribution<double> N(0.0, 1.0);

    std::vector<MarketPoint> data;
    data.reserve(T);

    double price = 100.0;

    for (int t = 0; t < T; ++t) {
        Regime regime;
        if (t < T/3) regime = Regime::TREND;
        else if (t < 2*T/3) regime = Regime::MEAN_REVERT;
        else regime = Regime::NOISE;

        double mu = 0.0;
        double sigma = 0.01;

        if (regime == Regime::TREND) mu = 0.001;
        if (regime == Regime::MEAN_REVERT) mu = -0.001 * (price - 100.0);
        if (regime == Regime::NOISE) mu = 0.0;

        double ret = mu + sigma * N(rng);
        price *= std::exp(ret);

        data.push_back({price, ret, regime});
    }

    return data;
}

int main() {
    auto market = generate_market(3000);

    for (const auto& p : market) {
        std::cout << p.price << " " << p.ret << "\n";
    }
    return 0;
}
