#include "utilities.h"

void prettyPrint(Bitboard bb) {
    for (int rank = 7; rank >= 0; rank--) {
        for (int file = 0; file < 8; file++) {
            Bitboard squareBB = (Bitboard)1 << (rank * 8 + file);
            if (bb & squareBB) std::cout << "1 ";
            else std::cout << "0 ";
        }
        std::cout << '\n';
    }
    std::cout << "\n\n";
}
