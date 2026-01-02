#include <cmath>
#include <iostream>
#include <random>

enum class PosState { FLAT, LONG, SHORT };

struct LOB {
    double bid_price;
    double ask_price;
    int bid_qty;
    int ask_qty;
};

int main(){
    std::mt19937 rng(123);
    std::poisson_distribution<int> arrivals(5);
    std::uniform_int_distribution<int> side(0,1);

    LOB book{100.0, 100.1, 100, 100};

    PosState pos = PosState::FLAT;
    int hold = 0;

    const double theta = 0.6;
    const double theta_exit = 0.2;
    const int max_hold = 50;

    for(int t=0;t<5000;t++){
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

        double I = (book.bid_qty - book.ask_qty) /
                   double(book.bid_qty + book.ask_qty);

        if(pos == PosState::FLAT){
            if(I > theta){
                pos = PosState::LONG;
                hold = 0;
            } else if(I < -theta){
                pos = PosState::SHORT;
                hold = 0;
            }
        } else {
            hold++;
            if(std::fabs(I) < theta_exit || hold > max_hold){
                pos = PosState::FLAT;
            }
        }

        if(t % 500 == 0){
            std::cout << "t=" << t
                      << " I=" << I
                      << " pos=" << int(pos)
                      << "\n";
        }
    }

    return 0;
}
