#include "board.h"
#include "fen.h"
#include "movegen.h"
#include "utilities.h"
#include <iostream>

int main(int argc, char** argv) {
    int depth = 3;
    if (argc > 1) {
        depth = std::atoi(argv[1]);
    }
    Fen fen = parseFEN("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");
    prettyPrint(fen.board);
    std::cout << "White turn: " << (fen.whiteTurn ? "true" : "false") << '\n';
    std::cout << "White kingside castling right: " << (fen.board.whiteKingsideCastlingRight ? "true" : "false") << '\n';
    std::cout << "White queenside castling right: " << (fen.board.whiteQueensideCastlingRight ? "true" : "false") << '\n';
    std::cout << "Black kingside castling right: " << (fen.board.blackKingsideCastlingRight ? "true" : "false") << '\n';
    std::cout << "Black queenside castling right: " << (fen.board.blackQueensideCastlingRight ? "true" : "false") << '\n';
    std::cout << "En passant square index: " << (int)fen.board.enPassant << '\n';
    std::cout << "Halfmove clock: " << fen.halfmoveClock << '\n';
    std::cout << "Fullmove number: " << fen.fullmoveNumber << '\n';
    std::cout << "perftest(" << depth << "): " << perftest(fen.board, depth, true) << '\n';
    return 0;
}
