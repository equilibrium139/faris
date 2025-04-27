#include "board.h"
#include "movegen.h"
#include "utilities.h"
#include <iostream>

int main() {
    Board board;
    // board.whitePawns &= 0b0000000011111111ULL; // clear pawns
    prettyPrint(board.allPieces());
    int count = 0;
    std::vector<Board> moves = genMoves(board, false);
    // count += moves.size();
    for (const Board& move : moves) {
        auto moves = genMoves(move, true);
        count += moves.size();
    }
    std::cout << "Number of moves: " << count << '\n';
    return 0;
}
