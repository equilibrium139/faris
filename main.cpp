#include "board.h"
#include "movegen.h"
#include "utilities.h"
#include <iostream>

int main(int argc, char** argv) {
    int depth = 7;
    if (argc > 1) {
        depth = std::atoi(argv[1]);
    }
    Board board;
    // board.whitePawns &= 0b0000000011111111ULL; // clear pawns
    std::cout << "perftest(" << depth << "): " << perftest(board, depth, true) << '\n';
    return 0;
}
