#include <iostream>
#include "board.h"
#include "movegen.h"
#include "utilities.h"

int main() {
    Board board;
    prettyPrint(board.allPieces());
    std::vector<Board> moves = genMoves(board, true);
    std::cout << "Number of moves: " << moves.size() << '\n';
    return 0; 
}
