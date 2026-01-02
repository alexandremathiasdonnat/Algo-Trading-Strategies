#include <iostream>
#include <random>
#include <iomanip>

int main() {
    const int N = 1000;
    const double order_size = 1000;
    const double base_price = 100.0;

    const double spread = 0.01;
    const double impact_coeff = 0.00005;
    const double sigma = 0.015;

    std::mt19937 rng(123);
    std::normal_distribution<double> noise(0.0, sigma);

    double total_slippage = 0.0;

    for (int i = 0; i < N; ++i) {
        double price_move = noise(rng);
        double market_price = base_price * std::exp(price_move);

        double impact = impact_coeff * order_size;
        double exec_price = market_price + spread + impact;

        double slippage = exec_price - base_price;
        total_slippage += slippage;
    }

    std::cout << "Slippage model results\n";
    std::cout << "Average slippage per trade: "
              << std::fixed << std::setprecision(6)
              << total_slippage / N << "\n";

    return 0;
}
