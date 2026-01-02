#include <iostream>
#include <random>

struct LOB {
    double bid_price;
    double ask_price;
    int bid_qty;
    int ask_qty;
};

int main() {
    std::mt19937 rng(42);
    std::poisson_distribution<int> arrivals(5);
    std::uniform_int_distribution<int> side(0,1);

    LOB book{100.0, 100.1, 100, 100};

    for(int t=0; t<5000; ++t){
        int events = arrivals(rng);
        for(int i=0;i<events;i++){
            bool buy = side(rng);

            if(buy){
                book.ask_qty -= 1;
                if(book.ask_qty <= 0){
                    book.ask_price += 0.1;
                    book.ask_qty = 100;
                }
            } else {
                book.bid_qty -= 1;
                if(book.bid_qty <= 0){
                    book.bid_price -= 0.1;
                    book.bid_qty = 100;
                }
            }
        }

        std::cout << book.bid_price << " "
                  << book.ask_price << " "
                  << book.bid_qty   << " "
                  << book.ask_qty   << "\n";
    }

    return 0;
}
