#include <iostream>
#include "board.h"
#include "movegen.h"
#include "utilities.h"

int main() {
    Board board;
    board.whitePawns &= 0b0000000011111111ULL; // clear pawns
    prettyPrint(board.allPieces());
    std::vector<Board> moves = genMoves(board, true);
    std::cout << "Number of moves: " << moves.size() << '\n';
    return 0; 
}
