#include <iostream>
#include <vector>
#include <random>
#include <iomanip>

int main() {
    const int T = 100;              // execution horizon
    const double total_qty = 10000; // parent order size
    const double slice_qty = total_qty / T;

    const double base_price = 100.0;
    const double sigma = 0.01;      // market volatility
    const double impact_coeff = 0.00002;

    std::mt19937 rng(42);
    std::normal_distribution<double> noise(0.0, sigma);

    double price = base_price;
    double executed_qty = 0.0;
    double cost = 0.0;

    for (int t = 0; t < T; ++t) {
        // market evolution
        price *= std::exp(noise(rng));

        // temporary impact
        double impact = impact_coeff * slice_qty;
        double execution_price = price + impact;

        executed_qty += slice_qty;
        cost += slice_qty * execution_price;

        std::cout << "t=" << t
                  << " exec_price=" << execution_price
                  << " cum_qty=" << executed_qty << "\n";
    }

    double avg_price = cost / total_qty;

    std::cout << "\nTWAP execution summary\n";
    std::cout << "Total quantity: " << total_qty << "\n";
    std::cout << "Average execution price: "
              << std::fixed << std::setprecision(4)
              << avg_price << "\n";
    std::cout << "Implementation shortfall: "
              << avg_price - base_price << "\n";

    return 0;
}
