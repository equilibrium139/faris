#include "utilities.h"
#include "board.h"
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

PieceType PieceTypeAt(Square square, const Board& board, Color color) {
    Bitboard squareBB = ToBitboard(square);
    for (int i = 0; i < 6; i++) {
        if (squareBB & board.bitboards2D[color][i]) {
            return PieceType(i);
        }
    }
    return PieceType::None;
}

// TODO: move to Board, or move all these low-level methods that operate on Board to a different file
PieceType RemovePiece(Square square, Board& board, Color color) {
    Bitboard squareBB = ToBitboard(square);
    PieceType removedPieceType = PieceType::None;
    for (int i = 0; i < 6; i++) {
        if (squareBB & board.bitboards2D[color][i]) {
            board.bitboards2D[color][i] &= ~squareBB;
            removedPieceType = PieceType(i);
            break;
        }
    }
    assert(removedPieceType != PieceType::None);
    return removedPieceType;
}

void MakeMove(const Move &move, Board &board, Color moveColor) {
    board.Move(move.type, moveColor, move.from, move.to);
    Color oppColor = ToggleColor(moveColor);
    Bitboard toBB = ToBitboard(move.to);
    if (move.capturedPieceType != PieceType::None) {
        if (board.enPassant && move.to == board.enPassant) {
            assert(move.capturedPieceType == PieceType::Pawn);
            constexpr int enPassantOffset[2] = { -8, 8 };
            Square epPawnSquare = move.to + enPassantOffset[moveColor];
            board.bitboards2D[oppColor][(int)move.capturedPieceType] &= ~ToBitboard(epPawnSquare);
        }
        else {
            board.bitboards2D[oppColor][(int)move.capturedPieceType] &= ~toBB;
        }
    }
    if (move.promotionType != PieceType::None) {
        board.PromotePawn(move.promotionType, moveColor, move.to);
    }
    if (move.type == PieceType::King && move.from == STARTING_KING_SQUARE[moveColor] ) {
        if (move.to == move.from + 2) { // short castle
            board.Move(PieceType::Rook, moveColor, move.from + 3, move.from + 1);
        }
        else if (move.to == move.from - 2) { // long castle
            board.Move(PieceType::Rook, moveColor, move.from - 4, move.from - 1);
        }
    }
    board.enPassant += move.enPassantDelta;
    board.shortCastlingRight[moveColor] = board.shortCastlingRight[moveColor] && !(move.flags & Move::RemovesShortCastlingRight);
    board.longCastlingRight[moveColor] = board.longCastlingRight[moveColor] && !(move.flags & Move::RemovesLongCastlingRight);
    board.shortCastlingRight[oppColor] = board.shortCastlingRight[oppColor] && !(move.flags & Move::RemovesOppShortCastlingRight);
    board.longCastlingRight[oppColor] = board.longCastlingRight[oppColor] && !(move.flags & Move::RemovesOppLongCastlingRight);
}

void UndoMove(const Move &move, Board &board, Color moveColor) {
    board.Move(move.type, moveColor, move.to, move.from);
    Bitboard toBB = ToBitboard(move.to);
    Color oppColor = ToggleColor(moveColor);
    Square originalEPSquare = board.enPassant - move.enPassantDelta;
    // TODO: handle enPassant capture properly
    if (move.capturedPieceType != PieceType::None) {
        if (move.to == originalEPSquare) {
            constexpr int enPassantOffset[2] = { -8, 8 };
            board.bitboards2D[oppColor][(int)move.capturedPieceType] |= ToBitboard(move.to + enPassantOffset[moveColor]);
        }
        else {
            board.bitboards2D[oppColor][(int)move.capturedPieceType] |= toBB;
        }
    }
    if (move.promotionType != PieceType::None) {
        board.bitboards2D[moveColor][(int)move.promotionType] &= ~toBB;
    }
    if (move.type == PieceType::King && move.from == STARTING_KING_SQUARE[moveColor] ) {
        if (move.to == move.from + 2) {
            board.Move(PieceType::Rook, moveColor, move.from + 1, move.from + 3); 
        }
        else if (move.to == move.from - 2) {
            board.Move(PieceType::Rook, moveColor, move.from - 1, move.from - 4);
        }
    }
    board.enPassant = originalEPSquare;
    board.shortCastlingRight[moveColor] = board.shortCastlingRight[moveColor] || (move.flags & Move::RemovesShortCastlingRight);
    board.longCastlingRight[moveColor] = board.longCastlingRight[moveColor] || (move.flags & Move::RemovesLongCastlingRight);
    board.shortCastlingRight[oppColor] = board.shortCastlingRight[oppColor] || (move.flags & Move::RemovesOppShortCastlingRight);
    board.longCastlingRight[oppColor] = board.longCastlingRight[oppColor] || (move.flags & Move::RemovesOppLongCastlingRight);
}
