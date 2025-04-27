#include "board.h"
#include "movegen.h"
#include "utilities.h"
#include <iostream>

int main() {
    Board board;
    // board.whitePawns &= 0b0000000011111111ULL; // clear pawns
    std::cout << "perftest(2): " << perftest(board, 2, true) << '\n';
    return 0;
}
