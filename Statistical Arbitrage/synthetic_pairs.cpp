#include <cmath>
#include <iostream>
#include <random>
#include <vector>

struct PairPoint {
    double x;
    double y;
};

std::vector<PairPoint> generate_cointegrated_pair(
    int T,
    double beta = 1.25,
    double noise_sigma_low = 0.5,
    double noise_sigma_high = 1.5,
    unsigned seed = 42
) {
    std::mt19937 rng(seed);
    std::normal_distribution<double> N(0.0, 1.0);

    std::vector<PairPoint> data;
    data.reserve(T);

    double x = 100.0;
    double y = beta * x;

    for (int t = 0; t < T; ++t) {
        // latent random walk for x
        double dx = 0.2 * N(rng);
        x += dx;

        // regime change in noise
        double sigma = (t < T/2) ? noise_sigma_low : noise_sigma_high;

        // y cointegrated with x
        double eps = sigma * N(rng);
        y = beta * x + eps;

        data.push_back({x, y});
    }

    return data;
}

int main() {
    auto series = generate_cointegrated_pair(3000);

    for (const auto& p : series) {
        std::cout << p.x << " " << p.y << "\n";
    }

    return 0;
}
