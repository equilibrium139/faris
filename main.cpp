#include "board.h"
#include <cassert>
#include "fen.h"
#include "movegen.h"
#include "perft.h"
#include "search.h"
#include "utilities.h"
#include <iostream>

int maxDepth;

void PrintMove(const Move& move) {
    switch (move.type) {
        case PieceType::Pawn:
            std::cout << "p";
            break;
        case PieceType::Knight:
            std::cout << "n";
            break;
        case PieceType::Bishop:
            std::cout << "b";
            break;
        case PieceType::Rook:
            std::cout << "r";
            break;
        case PieceType::Queen:
            std::cout << "q";
            break;
        case PieceType::King:
            std::cout << "k";
            break;
    }
    char fromFile = (move.from % 8) + 'a';
    char fromRank = move.from / 8 + '1';
    char toFile = (move.to % 8) + 'a';
    char toRank = move.to / 8 + '1';
    std::cout << fromFile << fromRank << toFile << toRank << '\n';
}

int main(int argc, char** argv) {
    maxDepth = 4;
    if (argc > 1) {
        maxDepth = std::atoi(argv[1]);
    }
    // std::string fenString = "r3k2r/Pppp1ppp/1b3nbN/nPB5/1qP1P3/P4N2/1p1P2PP/R2Q1RK1 w Qkq - 0 1";
    // std::string fenString = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R4RK1 w Qkq - 0 1";
    // Fen fen = ParseFen(fenString);
    // Fen fen = parseFEN("rnbqkb1r/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    // Fen fen = parseFEN("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1");
    Fen fen = ParseFen("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");
    // Fen fen = ParseFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"); // starting position
    Move move = Search(fen.board, maxDepth, fen.colorToMove);
    PrintMove(move); 
    // Fen fen = ParseFen("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");
    // Fen fen = parseFEN("n1n5/PPPk4/8/8/8/8/4Kppp/5N1N b - - 0 1"); // catch promotion bugs
    /*
    std::string boardToFen = ToFen(fen);
    Fen fenToBoard = ParseFen(boardToFen);
    assert(fenToBoard == fen);
    PrettyPrint(fen.board);
    std::cout << "Color to move: " << (fen.colorToMove ==  Color::White ? "white" : "black") << '\n';
    std::cout << "White kingside castling right: " << (fen.board.shortCastlingRight[White] ? "true" : "false") << '\n';
    std::cout << "White queenside castling right: " << (fen.board.longCastlingRight[White] ? "true" : "false") << '\n';
    std::cout << "Black kingside castling right: " << (fen.board.shortCastlingRight[Black] ? "true" : "false") << '\n';
    std::cout << "Black queenside castling right: " << (fen.board.longCastlingRight[Black] ? "true" : "false") << '\n';
    std::cout << "En passant square index: " << (int)fen.board.enPassant << '\n';
    std::cout << "Halfmove clock: " << fen.halfmoveClock << '\n';
    std::cout << "Fullmove number: " << fen.fullmoveNumber << '\n';
    std::uint64_t perftCount = perftest(fen.board, maxDepth, fen.colorToMove);
    std::cout << "perftest(" << maxDepth << "): " << perftCount << '\n';
    */
    /* if (enablePerftDiagnostics) {
        std::cout << "Castles: " << IncrementCastles() - 1 << '\n';
        std::cout << "Captures: " << IncrementCaptures() - 1 << '\n';
        std::cout << "En passant: " << IncrementEnPassant() - 1 << '\n';
    }
    */
    return 0;
}
