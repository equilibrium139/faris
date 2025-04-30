#include "board.h"
#include "movegen.h"
#include "utilities.h"
#include <iostream>

int main() {
    Board board;
    // board.whitePawns &= 0b0000000011111111ULL; // clear pawns
    int n = 5;
    std::cout << "perftest(" << n << "): " << perftest(board, n, true) << '\n';
    return 0;
}
