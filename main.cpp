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
    Fen fen = parseFEN("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
    // Fen fen = parseFEN("rnbqkb1r/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    // Fen fen = parseFEN("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1");
    // Fen fen = parseFEN("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");
    // Fen fen = parseFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"); // starting position
    // Fen fen = parseFEN("n1n5/PPPk4/8/8/8/8/4Kppp/5N1N b - - 0 1"); // catch promotion bugs
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
    std::cout << "Castles: " << incrementCastles() - 1 << '\n';
    std::cout << "Captures: " << incrementCaptures() - 1 << '\n';
    std::cout << "En passant: " << incrementEnPassant() - 1 << '\n';
    return 0;
}
