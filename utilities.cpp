#include "utilities.h"
#include <iostream>

void PrettyPrint(Bitboard bb) {
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

void PrettyPrint(const Board& board) {
    for (int rank = 7; rank >= 0; rank--) {
        for (int file = 0; file < 8; file++) {
            Bitboard squareBB = (Bitboard)1 << (rank * 8 + file);
            if (board.whitePawns & squareBB) std::cout << "P ";
            else if (board.whiteKnights & squareBB) std::cout << "N ";
            else if (board.whiteBishops & squareBB) std::cout << "B ";
            else if (board.whiteRooks & squareBB) std::cout << "R ";
            else if (board.whiteQueens & squareBB) std::cout << "Q ";
            else if (board.whiteKing & squareBB) std::cout << "K ";
            else if (board.blackPawns & squareBB) std::cout << "p ";
            else if (board.blackKnights & squareBB) std::cout << "n ";
            else if (board.blackBishops & squareBB) std::cout << "b ";
            else if (board.blackRooks & squareBB) std::cout << "r ";
            else if (board.blackQueens & squareBB) std::cout << "q ";
            else if (board.blackKing & squareBB) std::cout << "k ";
            else std::cout << ". ";
        }
        std::cout << '\n';
    }
    std::cout << "\n\n";
}

Piece PieceAt(int squareIndex, const Board &board) {
    Bitboard squareIndexBB = (Bitboard)1 << squareIndex;
    for (int i = 0; i < COUNT_BITBOARDS; i++) {
        if (squareIndexBB & board.bitboards[i]) {
            return i < 6 ? Piece{PieceType(i), Color::White} : Piece{PieceType(i - 6), Color::Black};
        }
    }
    return {PieceType::None};
}

// TODO: use uint8_t for squareIndex
// TODO: move to Board, or move all these low-level methods that operate on Board to a different file
static void RemovePiece(int squareIndex, std::span<Bitboard, 6> pieces) {
    assert(squareIndex >= 0 && squareIndex < 64);
    Bitboard captureSquareBB = (Bitboard)1 << squareIndex;
    for (Bitboard& pieceBoard : pieces) {
        if (pieceBoard & captureSquareBB) {
            pieceBoard &= ~captureSquareBB;
            return;
        }
    }
}


void RemovePiece(int squareIndex, Board& board, Color colorToRemove) {
    assert(squareIndex >= 0 && squareIndex < 64);
    RemovePiece(squareIndex, board.Bitboards(colorToRemove));
    board.whiteKingsideCastlingRight = board.whiteKingsideCastlingRight && squareIndex != 7;
    board.whiteQueensideCastlingRight = board.whiteQueensideCastlingRight && squareIndex != 0;
    board.blackKingsideCastlingRight = board.blackKingsideCastlingRight && squareIndex != 63;
    board.blackQueensideCastlingRight = board.blackQueensideCastlingRight && squareIndex != 56;
}
