#include "utilities.h"

void prettyPrint(Bitboard bb) {
    for (int i = 0; i < 64; i++) {
        if (i % 8 == 0) std::cout << '\n';
        if ((bb >> i) & 0b1) std::cout << "1 ";
        else std::cout << "0 ";    
    }
    std::cout << "\n\n";
}
